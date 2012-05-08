/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "KeePass1Reader.h"

#include <QtCore/QFile>
#include <QtCore/QTextCodec>

#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "format/KeePass1.h"
#include "keys/CompositeKey.h"
#include "streams/SymmetricCipherStream.h"

class KeePass1Key : public CompositeKey
{
public:
    virtual QByteArray rawKey() const;
    virtual void clear();
    void setPassword(const QByteArray& password);
    void setKeyfileData(const QByteArray& keyfileData);

private:
    QByteArray m_password;
    QByteArray m_keyfileData;
};


KeePass1Reader::KeePass1Reader()
    : m_error(false)
{
}

Database* KeePass1Reader::readDatabase(QIODevice* device, const QString& password,
                                       const QByteArray& keyfileData)
{
    QScopedPointer<Database> db(new Database());
    m_db = db.data();
    m_device = device;
    m_error = false;
    m_errorStr = QString();

    bool ok;

    quint32 signature1 = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok || signature1 != 0x9AA2D903) {
        raiseError(tr("Not a KeePass database."));
        return 0;
    }

    quint32 signature2 = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok || signature2 != 0xB54BFB65) {
        raiseError(tr("Not a KeePass database."));
        return 0;
    }

    m_encryptionFlags = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok || !(m_encryptionFlags & KeePass1::Rijndael || m_encryptionFlags & KeePass1::Twofish)) {
        raiseError(tr("Unsupported encryption algorithm."));
        return 0;
    }

    quint32 version = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok || (version & KeePass1::FILE_VERSION_CRITICAL_MASK)
            != (KeePass1::FILE_VERSION & KeePass1::FILE_VERSION_CRITICAL_MASK)) {
        raiseError(tr("Unsupported KeePass database version."));
        return 0;
    }

    m_masterSeed = m_device->read(16);
    if (m_masterSeed.size() != 16) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    m_encryptionIV = m_device->read(16);
    if (m_encryptionIV.size() != 16) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    quint32 numGroups = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    quint32 numEntries = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    m_contentHashHeader = m_device->read(32);
    if (m_contentHashHeader.size() != 32) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    m_transformSeed = m_device->read(32);
    if (m_transformSeed.size() != 32) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    m_transformRounds = Endian::readUInt32(m_device, KeePass1::BYTEORDER, &ok);
    if (!ok) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }
    m_db->setTransformRounds(m_transformRounds);

    qint64 contentPos = m_device->pos();

    QScopedPointer<SymmetricCipherStream> cipherStream(testKeys(password, keyfileData, contentPos));

    if (!cipherStream) {
        // TODO error
        Q_ASSERT(false);
        return 0;
    }

    return db.take();
}

Database* KeePass1Reader::readDatabase(const QString& filename, const QString& password,
                                       const QByteArray& keyfileData)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        raiseError(file.errorString());
        return 0;
    }

    QScopedPointer<Database> db(readDatabase(&file, password, keyfileData));

    if (file.error() != QFile::NoError) {
        raiseError(file.errorString());
        return 0;
    }

    return db.take();
}

bool KeePass1Reader::hasError()
{
    return m_error;
}

QString KeePass1Reader::errorString()
{
    return m_errorStr;
}

SymmetricCipherStream* KeePass1Reader::testKeys(const QString& password, const QByteArray& keyfileData,
                                                qint64 contentPos)
{
    QList<PasswordEncoding> encodings;
    encodings << Windows1252 << Latin1 << UTF8;

    QScopedPointer<SymmetricCipherStream> cipherStream;
    QByteArray passwordData;
    QTextCodec* codec = QTextCodec::codecForName("Windows-1252");
    QByteArray passwordDataCorrect = codec->fromUnicode(password);

    Q_FOREACH (PasswordEncoding encoding, encodings) {
        if (encoding == Windows1252) {
            passwordData = passwordDataCorrect;
        }
        else if (encoding == Latin1) {
            // KeePassX used Latin-1 encoding for passwords until version 0.3.1
            // but KeePass/Win32 uses Windows Codepage 1252.
            passwordData = password.toLatin1();

            if (passwordData == passwordDataCorrect) {
                continue;
            }
            else {
                qWarning("Testing password encoded as Latin-1.");
            }
        }
        else if (encoding == UTF8) {
            // KeePassX used UTF-8 encoding for passwords until version 0.2.2
            // but KeePass/Win32 uses Windows Codepage 1252.
            passwordData = password.toUtf8();

            if (passwordData == passwordDataCorrect) {
                continue;
            }
            else {
                qWarning("Testing password encoded as UTF-8.");
            }
        }

        QByteArray finalKey = key(passwordData, keyfileData);
        if (m_encryptionFlags & KeePass1::Rijndael) {
            cipherStream.reset(new SymmetricCipherStream(m_device, SymmetricCipher::Aes256,
                    SymmetricCipher::Cbc, SymmetricCipher::Decrypt, finalKey, m_encryptionIV));
        }
        else {
            // TODO twofish
        }

        cipherStream->open(QIODevice::ReadOnly);

        bool success = verifyKey(cipherStream.data());

        cipherStream->reset();
        cipherStream->close();
        if (!m_device->seek(contentPos)) {
            // TODO error
            Q_ASSERT(false);
            return 0;
        }
        cipherStream->open(QIODevice::ReadOnly);

        if (success) {
            break;
        }
        else {
            cipherStream.reset();
        }
    }

    return cipherStream.take();
}

QByteArray KeePass1Reader::key(const QByteArray& password, const QByteArray& keyfileData)
{
    KeePass1Key key;
    key.setPassword(password);
    key.setKeyfileData(keyfileData);

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(m_masterSeed);
    hash.addData(key.transform(m_transformSeed, m_transformRounds));
    return hash.result();
}

bool KeePass1Reader::verifyKey(SymmetricCipherStream* cipherStream)
{
    CryptoHash contentHash(CryptoHash::Sha256);
    QByteArray buffer;
    buffer.resize(16384);
    qint64 readResult;
    do {
        readResult = cipherStream->read(buffer.data(), buffer.size());
        if (readResult > 0) {
            if (readResult != buffer.size()) {
                buffer.resize(readResult);
            }
            qDebug("read %d", buffer.size());
            contentHash.addData(buffer);
        }
    } while (readResult == buffer.size());

    return contentHash.result() == m_contentHashHeader;
}

void KeePass1Reader::raiseError(const QString& str)
{
    m_error = true;
    m_errorStr = str;
}


QByteArray KeePass1Key::rawKey() const
{
    if (m_keyfileData.isEmpty()) {
        return CryptoHash::hash(m_password, CryptoHash::Sha256);
    }
    else if (m_password.isEmpty()) {
        return m_keyfileData;
    }
    else {
        CryptoHash keyHash(CryptoHash::Sha256);
        keyHash.addData(m_password);
        keyHash.addData(m_keyfileData);
        return keyHash.result();
    }
}

void KeePass1Key::clear()
{
    CompositeKey::clear();

    m_password.clear();
    m_keyfileData.clear();
}

void KeePass1Key::setPassword(const QByteArray& password)
{
    m_password = password;
}

void KeePass1Key::setKeyfileData(const QByteArray& keyfileData)
{
    m_keyfileData = keyfileData;
}
