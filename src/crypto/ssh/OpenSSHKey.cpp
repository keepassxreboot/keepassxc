/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OpenSSHKey.h"

#include "core/Tools.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/ssh/ASN1Key.h"
#include "crypto/ssh/BinaryStream.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QStringList>

#include <gcrypt.h>

const QString OpenSSHKey::TYPE_DSA_PRIVATE = "DSA PRIVATE KEY";
const QString OpenSSHKey::TYPE_RSA_PRIVATE = "RSA PRIVATE KEY";
const QString OpenSSHKey::TYPE_RSA_PUBLIC = "RSA PUBLIC KEY";
const QString OpenSSHKey::TYPE_OPENSSH_PRIVATE = "OPENSSH PRIVATE KEY";

namespace
{
    QPair<QString, QList<QByteArray>> binaryDeserialize(const QByteArray& serialized)
    {
        if (serialized.isEmpty()) {
            return {};
        }
        QBuffer buffer;
        buffer.setData(serialized);
        buffer.open(QBuffer::ReadOnly);
        BinaryStream stream(&buffer);
        QString type;
        stream.readString(type);
        QByteArray temp;
        QList<QByteArray> data;
        while (stream.readString(temp)) {
            data << temp;
        }
        return ::qMakePair(type, data);
    }

    QByteArray binarySerialize(const QString& type, const QList<QByteArray>& data)
    {
        if (type.isEmpty() && data.isEmpty()) {
            return {};
        }
        QByteArray buffer;
        BinaryStream stream(&buffer);
        stream.writeString(type);
        for (const QByteArray& part : data) {
            stream.writeString(part);
        }
        return buffer;
    }
} // namespace

// bcrypt_pbkdf.cpp
int bcrypt_pbkdf(const QByteArray& pass, const QByteArray& salt, QByteArray& key, quint32 rounds);

OpenSSHKey OpenSSHKey::generate(bool secure)
{
    enum Index
    {
        Params,
        CombinedKey,
        PrivateKey,
        PublicKey,

        Private_N,
        Private_E,
        Private_D,
        Private_P,
        Private_Q,
        Private_U, // private key
        Public_N,
        Public_E,
    };

    Tools::Map<Index, gcry_mpi_t, &gcry_mpi_release> mpi;
    Tools::Map<Index, gcry_sexp_t, &gcry_sexp_release> sexp;
    gcry_error_t rc = GPG_ERR_NO_ERROR;
    rc = gcry_sexp_build(&sexp[Params],
                         NULL,
                         secure ? "(genkey (rsa (nbits 4:2048)))" : "(genkey (rsa (transient-key) (nbits 4:2048)))");
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not create ssh key" << gcry_err_code(rc);
        return OpenSSHKey();
    }

    rc = gcry_pk_genkey(&sexp[CombinedKey], sexp[Params]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not create ssh key" << gcry_err_code(rc);
        return OpenSSHKey();
    }

    sexp[PrivateKey] = gcry_sexp_find_token(sexp[CombinedKey], "private-key", 0);
    sexp[PublicKey] = gcry_sexp_find_token(sexp[CombinedKey], "public-key", 0);

    sexp[Private_N] = gcry_sexp_find_token(sexp[PrivateKey], "n", 1);
    mpi[Private_N] = gcry_sexp_nth_mpi(sexp[Private_N], 1, GCRYMPI_FMT_USG);
    sexp[Private_E] = gcry_sexp_find_token(sexp[PrivateKey], "e", 1);
    mpi[Private_E] = gcry_sexp_nth_mpi(sexp[Private_E], 1, GCRYMPI_FMT_USG);
    sexp[Private_D] = gcry_sexp_find_token(sexp[PrivateKey], "d", 1);
    mpi[Private_D] = gcry_sexp_nth_mpi(sexp[Private_D], 1, GCRYMPI_FMT_USG);
    sexp[Private_Q] = gcry_sexp_find_token(sexp[PrivateKey], "q", 1);
    mpi[Private_Q] = gcry_sexp_nth_mpi(sexp[Private_Q], 1, GCRYMPI_FMT_USG);
    sexp[Private_P] = gcry_sexp_find_token(sexp[PrivateKey], "p", 1);
    mpi[Private_P] = gcry_sexp_nth_mpi(sexp[Private_P], 1, GCRYMPI_FMT_USG);
    sexp[Private_U] = gcry_sexp_find_token(sexp[PrivateKey], "u", 1);
    mpi[Private_U] = gcry_sexp_nth_mpi(sexp[Private_U], 1, GCRYMPI_FMT_USG);

    sexp[Public_N] = gcry_sexp_find_token(sexp[PublicKey], "n", 1);
    mpi[Public_N] = gcry_sexp_nth_mpi(sexp[Public_N], 1, GCRYMPI_FMT_USG);
    sexp[Public_E] = gcry_sexp_find_token(sexp[PublicKey], "e", 1);
    mpi[Public_E] = gcry_sexp_nth_mpi(sexp[Public_E], 1, GCRYMPI_FMT_USG);

    QList<QByteArray> publicParts;
    QList<QByteArray> privateParts;
    Tools::Buffer buffer;
    gcry_mpi_format format = GCRYMPI_FMT_USG;
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_N]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_E]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_D]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_U]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_P]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Private_Q]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract private key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    privateParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Public_E]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract public key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    publicParts << buffer.content();

    buffer.clear();
    rc = gcry_mpi_aprint(format, &buffer.raw, &buffer.size, mpi[Public_N]);
    if (rc != GPG_ERR_NO_ERROR) {
        qWarning() << "Could not extract public key part" << gcry_err_code(rc);
        return OpenSSHKey();
    }
    publicParts << buffer.content();
    OpenSSHKey key;
    key.m_rawType = OpenSSHKey::TYPE_RSA_PRIVATE;
    key.setType("ssh-rsa");
    key.setPublicData(publicParts);
    key.setPrivateData(privateParts);
    key.setComment("");
    return key;
}

OpenSSHKey::OpenSSHKey(QObject* parent)
    : QObject(parent)
    , m_type(QString())
    , m_cipherName(QString("none"))
    , m_kdfName(QString("none"))
    , m_kdfOptions(QByteArray())
    , m_rawType(QString())
    , m_rawData(QByteArray())
    , m_rawPublicData(QList<QByteArray>())
    , m_rawPrivateData(QList<QByteArray>())
    , m_comment(QString())
    , m_error(QString())
{
}

OpenSSHKey::OpenSSHKey(const OpenSSHKey& other)
    : QObject(nullptr)
    , m_type(other.m_type)
    , m_cipherName(other.m_cipherName)
    , m_kdfName(other.m_kdfName)
    , m_kdfOptions(other.m_kdfOptions)
    , m_rawType(other.m_rawType)
    , m_rawData(other.m_rawData)
    , m_rawPublicData(other.m_rawPublicData)
    , m_rawPrivateData(other.m_rawPrivateData)
    , m_comment(other.m_comment)
    , m_error(other.m_error)
{
}

bool OpenSSHKey::operator==(const OpenSSHKey& other) const
{
    // close enough for now
    return (fingerprint() == other.fingerprint());
}

const QString OpenSSHKey::cipherName() const
{
    return m_cipherName;
}

const QString OpenSSHKey::type() const
{
    return m_type;
}

int OpenSSHKey::keyLength() const
{
    if (m_type == "ssh-dss" && m_rawPublicData.length() == 4) {
        return (m_rawPublicData[0].length() - 1) * 8;
    } else if (m_type == "ssh-rsa" && m_rawPublicData.length() == 2) {
        return (m_rawPublicData[1].length() - 1) * 8;
    } else if (m_type.startsWith("ecdsa-sha2-") && m_rawPublicData.length() == 2) {
        return (m_rawPublicData[1].length() - 1) * 4;
    } else if (m_type == "ssh-ed25519" && m_rawPublicData.length() == 1) {
        return m_rawPublicData[0].length() * 8;
    }
    return 0;
}

const QString OpenSSHKey::fingerprint(QCryptographicHash::Algorithm algo) const
{
    if (m_rawPublicData.isEmpty()) {
        return {};
    }

    QByteArray publicKey;
    BinaryStream stream(&publicKey);

    stream.writeString(m_type);

    for (const QByteArray& ba : m_rawPublicData) {
        stream.writeString(ba);
    }

    QByteArray rawHash = QCryptographicHash::hash(publicKey, algo);

    if (algo == QCryptographicHash::Md5) {
        QString md5Hash = QString::fromLatin1(rawHash.toHex());
        QStringList md5HashParts;
        for (int i = 0; i < md5Hash.length(); i += 2) {
            md5HashParts.append(md5Hash.mid(i, 2));
        }
        return "MD5:" + md5HashParts.join(':');
    } else if (algo == QCryptographicHash::Sha256) {
        return "SHA256:" + QString::fromLatin1(rawHash.toBase64(QByteArray::OmitTrailingEquals));
    }

    return "HASH:" + QString::fromLatin1(rawHash.toHex());
}

const QString OpenSSHKey::comment() const
{
    return m_comment;
}

const QString OpenSSHKey::privateKey() const
{
    if (m_rawPrivateData.isEmpty()) {
        return {};
    }

    QByteArray privateKey;
    BinaryStream stream(&privateKey);

    stream.writeString(m_type);

    for (QByteArray ba : m_rawPrivateData) {
        stream.writeString(ba);
    }

    return m_type + " " + QString::fromLatin1(privateKey.toBase64()) + " " + m_comment;
}

const QString OpenSSHKey::publicKey() const
{
    if (m_rawPublicData.isEmpty()) {
        return {};
    }

    QByteArray publicKey;
    BinaryStream stream(&publicKey);

    stream.writeString(m_type);

    for (QByteArray ba : m_rawPublicData) {
        stream.writeString(ba);
    }

    return m_type + " " + QString::fromLatin1(publicKey.toBase64()) + " " + m_comment;
}

const QString OpenSSHKey::errorString() const
{
    return m_error;
}

void OpenSSHKey::setType(const QString& type)
{
    m_type = type;
}

void OpenSSHKey::setPublicData(const QList<QByteArray>& data)
{
    m_rawPublicData = data;
}

void OpenSSHKey::setPrivateData(const QList<QByteArray>& data)
{
    m_rawPrivateData = data;
}

void OpenSSHKey::setComment(const QString& comment)
{
    m_comment = comment;
}

void OpenSSHKey::clearPrivate()
{
    m_rawData.clear();
    m_rawPrivateData.clear();
}

bool OpenSSHKey::extractPEM(const QByteArray& in, QByteArray& out)
{
    QString pem = QString::fromLatin1(in);
    QStringList rows = pem.split(QRegularExpression("(?:\r?\n|\r)"), QString::SkipEmptyParts);

    if (rows.length() < 3) {
        m_error = tr("Invalid key file, expecting an OpenSSH key");
        return false;
    }

    QString begin = rows.first();
    QString end = rows.last();

    QRegularExpressionMatch beginMatch = QRegularExpression("^-----BEGIN ([^\\-]+)-----$").match(begin);
    QRegularExpressionMatch endMatch = QRegularExpression("^-----END ([^\\-]+)-----$").match(end);

    if (!beginMatch.hasMatch() || !endMatch.hasMatch()) {
        m_error = tr("Invalid key file, expecting an OpenSSH key");
        return false;
    }

    if (beginMatch.captured(1) != endMatch.captured(1)) {
        m_error = tr("PEM boundary mismatch");
        return false;
    }

    m_rawType = beginMatch.captured(1);

    rows.removeFirst();
    rows.removeLast();

    QRegularExpression keyValueExpr = QRegularExpression("^([A-Za-z0-9-]+): (.+)$");
    QMap<QString, QString> pemOptions;

    do {
        QRegularExpressionMatch keyValueMatch = keyValueExpr.match(rows.first());

        if (!keyValueMatch.hasMatch()) {
            break;
        }

        pemOptions.insert(keyValueMatch.captured(1), keyValueMatch.captured(2));

        rows.removeFirst();
    } while (!rows.isEmpty());

    if (pemOptions.value("Proc-Type").compare("4,encrypted", Qt::CaseInsensitive) == 0) {
        m_kdfName = "md5";
        m_cipherName = pemOptions.value("DEK-Info").section(",", 0, 0);
        m_cipherIV = QByteArray::fromHex(pemOptions.value("DEK-Info").section(",", 1, 1).toLatin1());
    }

    out = QByteArray::fromBase64(rows.join("").toLatin1());

    if (out.isEmpty()) {
        m_error = tr("Base64 decoding failed");
        return false;
    }

    return true;
}

bool OpenSSHKey::parsePKCS1PEM(const QByteArray& in)
{
    QByteArray data;

    if (!extractPEM(in, data)) {
        return false;
    }

    if (m_rawType == TYPE_DSA_PRIVATE || m_rawType == TYPE_RSA_PRIVATE || m_rawType == TYPE_RSA_PUBLIC) {
        m_rawData = data;
    } else if (m_rawType == TYPE_OPENSSH_PRIVATE) {
        BinaryStream stream(&data);

        QByteArray magic;
        magic.resize(15);

        if (!stream.read(magic)) {
            m_error = tr("Key file way too small.");
            return false;
        }

        if (QString::fromLatin1(magic) != "openssh-key-v1") {
            m_error = tr("Key file magic header id invalid");
            return false;
        }

        stream.readString(m_cipherName);
        stream.readString(m_kdfName);
        stream.readString(m_kdfOptions);

        quint32 numberOfKeys;
        stream.read(numberOfKeys);

        if (numberOfKeys == 0) {
            m_error = tr("Found zero keys");
            return false;
        }

        for (quint32 i = 0; i < numberOfKeys; ++i) {
            QByteArray publicKey;
            if (!stream.readString(publicKey)) {
                m_error = tr("Failed to read public key.");
                return false;
            }

            if (i == 0) {
                BinaryStream publicStream(&publicKey);
                if (!readPublic(publicStream)) {
                    return false;
                }
            }
        }

        // padded list of keys
        if (!stream.readString(m_rawData)) {
            m_error = tr("Corrupted key file, reading private key failed");
            return false;
        }
    } else {
        m_error = tr("Unsupported key type: %1").arg(m_rawType);
        return false;
    }

    // load private if no encryption
    if (!encrypted()) {
        return openKey();
    }

    return true;
}

bool OpenSSHKey::encrypted() const
{
    return (m_cipherName != "none");
}

bool OpenSSHKey::openKey(const QString& passphrase)
{
    QScopedPointer<SymmetricCipher> cipher;

    if (!m_rawPrivateData.isEmpty()) {
        return true;
    }

    if (m_rawData.isEmpty()) {
        m_error = tr("No private key payload to decrypt");
        return false;
    }

    if (m_cipherName.compare("aes-128-cbc", Qt::CaseInsensitive) == 0) {
        cipher.reset(new SymmetricCipher(SymmetricCipher::Aes128, SymmetricCipher::Cbc, SymmetricCipher::Decrypt));
    } else if (m_cipherName == "aes256-cbc" || m_cipherName.compare("aes-256-cbc", Qt::CaseInsensitive) == 0) {
        cipher.reset(new SymmetricCipher(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt));
    } else if (m_cipherName == "aes256-ctr" || m_cipherName.compare("aes-256-ctr", Qt::CaseInsensitive) == 0) {
        cipher.reset(new SymmetricCipher(SymmetricCipher::Aes256, SymmetricCipher::Ctr, SymmetricCipher::Decrypt));
    } else if (m_cipherName != "none") {
        m_error = tr("Unknown cipher: %1").arg(m_cipherName);
        return false;
    }

    if (m_kdfName == "bcrypt") {
        if (!cipher) {
            m_error = tr("Trying to run KDF without cipher");
            return false;
        }

        if (passphrase.isEmpty()) {
            m_error = tr("Passphrase is required to decrypt this key");
            return false;
        }

        BinaryStream optionStream(&m_kdfOptions);

        QByteArray salt;
        quint32 rounds;

        optionStream.readString(salt);
        optionStream.read(rounds);

        QByteArray decryptKey;
        decryptKey.fill(0, cipher->keySize() + cipher->blockSize());

        QByteArray phraseData = passphrase.toUtf8();
        if (bcrypt_pbkdf(phraseData, salt, decryptKey, rounds) < 0) {
            m_error = tr("Key derivation failed, key file corrupted?");
            return false;
        }

        QByteArray keyData, ivData;
        keyData.setRawData(decryptKey.data(), cipher->keySize());
        ivData.setRawData(decryptKey.data() + cipher->keySize(), cipher->blockSize());

        cipher->init(keyData, ivData);

        if (!cipher->init(keyData, ivData)) {
            m_error = cipher->errorString();
            return false;
        }
    } else if (m_kdfName == "md5") {
        if (m_cipherIV.length() < 8) {
            m_error = tr("Cipher IV is too short for MD5 kdf");
            return false;
        }

        QByteArray keyData;
        QByteArray mdBuf;
        do {
            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData(mdBuf);
            hash.addData(passphrase.toUtf8());
            hash.addData(m_cipherIV.data(), 8);
            mdBuf = hash.result();
            keyData.append(mdBuf);
        } while (keyData.size() < cipher->keySize());

        if (keyData.size() > cipher->keySize()) {
            // If our key size isn't a multiple of 16 (e.g. AES-192 or something),
            // then we will need to truncate it.
            keyData.resize(cipher->keySize());
        }

        if (!cipher->init(keyData, m_cipherIV)) {
            m_error = cipher->errorString();
            return false;
        }
    } else if (m_kdfName != "none") {
        m_error = tr("Unknown KDF: %1").arg(m_kdfName);
        return false;
    }

    QByteArray rawData = m_rawData;

    if (cipher && cipher->isInitalized()) {
        bool ok = false;
        rawData = cipher->process(rawData, &ok);
        if (!ok) {
            m_error = tr("Decryption failed, wrong passphrase?");
            return false;
        }
    }

    if (m_rawType == TYPE_DSA_PRIVATE) {
        if (!ASN1Key::parseDSA(rawData, *this)) {
            m_error = tr("Decryption failed, wrong passphrase?");
            return false;
        }

        return true;
    } else if (m_rawType == TYPE_RSA_PRIVATE) {
        if (!ASN1Key::parsePrivateRSA(rawData, *this)) {
            m_error = tr("Decryption failed, wrong passphrase?");
            return false;
        }
        return true;
    } else if (m_rawType == TYPE_RSA_PUBLIC) {
        if (!ASN1Key::parsePublicRSA(rawData, *this)) {
            m_error = tr("Decryption failed, wrong passphrase?");
            return false;
        }
        return true;
    } else if (m_rawType == TYPE_OPENSSH_PRIVATE) {
        BinaryStream keyStream(&rawData);

        quint32 checkInt1;
        quint32 checkInt2;

        keyStream.read(checkInt1);
        keyStream.read(checkInt2);

        if (checkInt1 != checkInt2) {
            m_error = tr("Decryption failed, wrong passphrase?");
            return false;
        }

        return readPrivate(keyStream);
    }

    m_error = tr("Unsupported key type: %1").arg(m_rawType);
    return false;
}

bool OpenSSHKey::readPublic(BinaryStream& stream)
{
    m_rawPublicData.clear();

    if (!stream.readString(m_type)) {
        m_error = tr("Unexpected EOF while reading public key");
        return false;
    }

    int keyParts;
    if (m_type == "ssh-dss") {
        keyParts = 4;
    } else if (m_type == "ssh-rsa") {
        keyParts = 2;
    } else if (m_type.startsWith("ecdsa-sha2-")) {
        keyParts = 2;
    } else if (m_type == "ssh-ed25519") {
        keyParts = 1;
    } else {
        m_error = tr("Unknown key type: %1").arg(m_type);
        return false;
    }

    for (int i = 0; i < keyParts; ++i) {
        QByteArray t;

        if (!stream.readString(t)) {
            m_error = tr("Unexpected EOF while reading public key");
            return false;
        }

        m_rawPublicData.append(t);
    }

    return true;
}

bool OpenSSHKey::readPrivate(BinaryStream& stream)
{
    m_rawPrivateData.clear();

    if (!stream.readString(m_type)) {
        m_error = tr("Unexpected EOF while reading private key");
        return false;
    }

    int keyParts;
    if (m_type == "ssh-dss") {
        keyParts = 5;
    } else if (m_type == "ssh-rsa") {
        keyParts = 6;
    } else if (m_type.startsWith("ecdsa-sha2-")) {
        keyParts = 3;
    } else if (m_type == "ssh-ed25519") {
        keyParts = 2;
    } else {
        m_error = tr("Unknown key type: %1").arg(m_type);
        return false;
    }

    for (int i = 0; i < keyParts; ++i) {
        QByteArray t;

        if (!stream.readString(t)) {
            m_error = tr("Unexpected EOF while reading private key");
            return false;
        }

        m_rawPrivateData.append(t);
    }

    if (!stream.readString(m_comment)) {
        m_error = tr("Unexpected EOF while reading private key");
        return false;
    }

    return true;
}

bool OpenSSHKey::writePublic(BinaryStream& stream)
{
    if (m_rawPublicData.isEmpty()) {
        m_error = tr("Can't write public key as it is empty");
        return false;
    }

    if (!stream.writeString(m_type)) {
        m_error = tr("Unexpected EOF when writing public key");
        return false;
    }

    for (QByteArray t : m_rawPublicData) {
        if (!stream.writeString(t)) {
            m_error = tr("Unexpected EOF when writing public key");
            return false;
        }
    }

    return true;
}

bool OpenSSHKey::writePrivate(BinaryStream& stream)
{
    if (m_rawPrivateData.isEmpty()) {
        m_error = tr("Can't write private key as it is empty");
        return false;
    }

    if (!stream.writeString(m_type)) {
        m_error = tr("Unexpected EOF when writing private key");
        return false;
    }

    for (QByteArray t : m_rawPrivateData) {
        if (!stream.writeString(t)) {
            m_error = tr("Unexpected EOF when writing private key");
            return false;
        }
    }

    if (!stream.writeString(m_comment)) {
        m_error = tr("Unexpected EOF when writing private key");
        return false;
    }

    return true;
}

QList<QByteArray> OpenSSHKey::publicParts() const
{
    return m_rawPublicData;
}

QList<QByteArray> OpenSSHKey::privateParts() const
{
    return m_rawPrivateData;
}

const QString& OpenSSHKey::privateType() const
{
    return m_rawType;
}

OpenSSHKey OpenSSHKey::restoreFromBinary(Type type, const QByteArray& serialized)
{
    OpenSSHKey key;
    auto data = binaryDeserialize(serialized);
    key.setType(data.first);
    switch (type) {
    case Public:
        key.setPublicData(data.second);
        break;
    case Private:
        key.setPrivateData(data.second);
        break;
    }
    return key;
}

QByteArray OpenSSHKey::serializeToBinary(Type type, const OpenSSHKey& key)
{
    Q_ASSERT(!key.encrypted());
    switch (type) {
    case Public:
        return binarySerialize(key.type(), key.publicParts());
    case Private:
        return binarySerialize(key.type(), key.privateParts());
    }
    return {};
}

uint qHash(const OpenSSHKey& key)
{
    return qHash(key.fingerprint());
}
