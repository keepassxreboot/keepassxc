/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Kdbx3Reader.h"

#include <QBuffer>
#include <QFile>
#include <QIODevice>

#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass1.h"
#include "format/KeePass2.h"
#include "format/KeePass2RandomStream.h"
#include "format/Kdbx3XmlReader.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/StoreDataStream.h"
#include "streams/SymmetricCipherStream.h"

Kdbx3Reader::Kdbx3Reader()
    : m_device(nullptr)
    , m_headerStream(nullptr)
    , m_headerEnd(false)
    , m_db(nullptr)
{
}

Database* Kdbx3Reader::readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase)
{
    QScopedPointer<Database> db(new Database());
    m_db = db.data();
    m_device = device;
    m_error = false;
    m_errorStr.clear();
    m_headerEnd = false;
    m_xmlData.clear();
    m_masterSeed.clear();
    m_encryptionIV.clear();
    m_streamStartBytes.clear();
    m_protectedStreamKey.clear();

    StoreDataStream headerStream(m_device);
    headerStream.open(QIODevice::ReadOnly);
    m_headerStream = &headerStream;

    bool ok;

    quint32 signature1 = Endian::readSizedInt<quint32>(m_headerStream, KeePass2::BYTEORDER, &ok);
    if (!ok || signature1 != KeePass2::SIGNATURE_1) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    quint32 signature2 = Endian::readSizedInt<quint32>(m_headerStream, KeePass2::BYTEORDER, &ok);
    if (ok && signature2 == KeePass1::SIGNATURE_2) {
        raiseError(tr("The selected file is an old KeePass 1 database (.kdb).\n\n"
                          "You can import it by clicking on Database > 'Import KeePass 1 database...'.\n"
                          "This is a one-way migration. You won't be able to open the imported "
                          "database with the old KeePassX 0.4 version."));
        return nullptr;
    } else if (!ok || signature2 != KeePass2::SIGNATURE_2) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    quint32 version = Endian::readSizedInt<quint32>(m_headerStream, KeePass2::BYTEORDER, &ok)
        & KeePass2::FILE_VERSION_CRITICAL_MASK;
    quint32 maxVersion = KeePass2::FILE_VERSION_3 & KeePass2::FILE_VERSION_CRITICAL_MASK;
    if (!ok || (version < KeePass2::FILE_VERSION_MIN) || (version > maxVersion)) {
        raiseError(tr("Unsupported KeePass KDBX 2 or 3 database version."));
        return nullptr;
    }

    while (readHeaderField() && !hasError()) {
    }

    headerStream.close();

    if (hasError()) {
        return nullptr;
    }

    // check if all required headers were present
    if (m_masterSeed.isEmpty() || m_encryptionIV.isEmpty()
        || m_streamStartBytes.isEmpty() || m_protectedStreamKey.isEmpty()
        || m_db->cipher().isNull()) {
        raiseError("missing database headers");
        return nullptr;
    }

    if (!m_db->setKey(key, false)) {
        raiseError(tr("Unable to calculate master key"));
        return nullptr;
    }

    if (m_db->challengeMasterSeed(m_masterSeed) == false) {
        raiseError(tr("Unable to issue challenge-response."));
        return nullptr;
    }

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(m_masterSeed);
    hash.addData(m_db->challengeResponseKey());
    hash.addData(m_db->transformedMasterKey());
    QByteArray finalKey = hash.result();

    SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
    SymmetricCipherStream cipherStream(m_device, cipher,
                                       SymmetricCipher::algorithmMode(cipher), SymmetricCipher::Decrypt);
    if (!cipherStream.init(finalKey, m_encryptionIV)) {
        raiseError(cipherStream.errorString());
        return nullptr;
    }
    if (!cipherStream.open(QIODevice::ReadOnly)) {
        raiseError(cipherStream.errorString());
        return nullptr;
    }

    QByteArray realStart = cipherStream.read(32);

    if (realStart != m_streamStartBytes) {
        raiseError(tr("Wrong key or database file is corrupt."));
        return nullptr;
    }

    HashedBlockStream hashedStream(&cipherStream);
    if (!hashedStream.open(QIODevice::ReadOnly)) {
        raiseError(hashedStream.errorString());
        return nullptr;
    }

    QIODevice* xmlDevice;
    QScopedPointer<QtIOCompressor> ioCompressor;

    if (m_db->compressionAlgo() == Database::CompressionNone) {
        xmlDevice = &hashedStream;
    } else {
        ioCompressor.reset(new QtIOCompressor(&hashedStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::ReadOnly)) {
            raiseError(ioCompressor->errorString());
            return nullptr;
        }
        xmlDevice = ioCompressor.data();
    }

    KeePass2RandomStream randomStream(KeePass2::ProtectedStreamAlgo::Salsa20);
    if (!randomStream.init(m_protectedStreamKey)) {
        raiseError(randomStream.errorString());
        return nullptr;
    }

    QScopedPointer<QBuffer> buffer;

    if (m_saveXml) {
        m_xmlData = xmlDevice->readAll();
        buffer.reset(new QBuffer(&m_xmlData));
        buffer->open(QIODevice::ReadOnly);
        xmlDevice = buffer.data();
    }

    Kdbx3XmlReader xmlReader;
    xmlReader.readDatabase(xmlDevice, m_db, &randomStream);

    if (xmlReader.hasError()) {
        raiseError(xmlReader.errorString());
        if (keepDatabase) {
            return db.take();
        } else {
            return nullptr;
        }
    }

    Q_ASSERT(version < 0x00030001 || !xmlReader.headerHash().isEmpty());

    if (!xmlReader.headerHash().isEmpty()) {
        QByteArray headerHash = CryptoHash::hash(headerStream.storedData(), CryptoHash::Sha256);
        if (headerHash != xmlReader.headerHash()) {
            raiseError("Header doesn't match hash");
            return nullptr;
        }
    }

    return db.take();
}

bool Kdbx3Reader::readHeaderField()
{
    QByteArray fieldIDArray = m_headerStream->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid header id size");
        return false;
    }
    char fieldID = fieldIDArray.at(0);

    bool ok;
    auto fieldLen = Endian::readSizedInt<quint16>(m_headerStream, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError("Invalid header field length");
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = m_headerStream->read(fieldLen);
        if (fieldData.size() != fieldLen) {
            raiseError("Invalid header data length");
            return false;
        }
    }

    switch (static_cast<KeePass2::HeaderFieldID>(fieldID)) {
    case KeePass2::HeaderFieldID::EndOfHeader:
        m_headerEnd = true;
        break;

    case KeePass2::HeaderFieldID::CipherID:
        setCipher(fieldData);
        break;

    case KeePass2::HeaderFieldID::CompressionFlags:
        setCompressionFlags(fieldData);
        break;

    case KeePass2::HeaderFieldID::MasterSeed:
        setMasterSeed(fieldData);
        break;

    case KeePass2::HeaderFieldID::TransformSeed:
        setTransformSeed(fieldData);
        break;

    case KeePass2::HeaderFieldID::TransformRounds:
        setTransformRounds(fieldData);
        break;

    case KeePass2::HeaderFieldID::EncryptionIV:
        setEncryptionIV(fieldData);
        break;

    case KeePass2::HeaderFieldID::ProtectedStreamKey:
        setProtectedStreamKey(fieldData);
        break;

    case KeePass2::HeaderFieldID::StreamStartBytes:
        setStreamStartBytes(fieldData);
        break;

    case KeePass2::HeaderFieldID::InnerRandomStreamID:
        setInnerRandomStreamID(fieldData);
        break;

    default:
        qWarning("Unknown header field read: id=%d", fieldID);
        break;
    }

    return !m_headerEnd;
}

void Kdbx3Reader::setCipher(const QByteArray& data)
{
    if (data.size() != Uuid::Length) {
        raiseError("Invalid cipher uuid length");
        return;
    }

    Uuid uuid(data);

    if (SymmetricCipher::cipherToAlgorithm(uuid) == SymmetricCipher::InvalidAlgorithm) {
        raiseError("Unsupported cipher");
        return;
    }
    m_db->setCipher(uuid);
}

void Kdbx3Reader::setCompressionFlags(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("Invalid compression flags length");
        return;
    }
    auto id = Endian::bytesToSizedInt<quint32>(data, KeePass2::BYTEORDER);

    if (id > Database::CompressionAlgorithmMax) {
        raiseError("Unsupported compression algorithm");
        return;
    }
    m_db->setCompressionAlgo(static_cast<Database::CompressionAlgorithm>(id));
}

void Kdbx3Reader::setMasterSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid master seed size");
        return;
    }
    m_masterSeed = data;
}

void Kdbx3Reader::setTransformSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid transform seed size");
        return;
    }

    auto kdf = m_db->kdf();
    if (!kdf.isNull()) {
        kdf->setSeed(data);
    }
}

void Kdbx3Reader::setTransformRounds(const QByteArray& data)
{
    if (data.size() != 8) {
        raiseError("Invalid transform rounds size");
        return;
    }

    auto rounds = Endian::bytesToSizedInt<quint64>(data, KeePass2::BYTEORDER);
    auto kdf = m_db->kdf();
    if (!kdf.isNull()) {
        kdf->setRounds(rounds);
    }
}

void Kdbx3Reader::setEncryptionIV(const QByteArray& data)
{
    m_encryptionIV = data;
}

void Kdbx3Reader::setProtectedStreamKey(const QByteArray& data)
{
    m_protectedStreamKey = data;
}

void Kdbx3Reader::setStreamStartBytes(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid start bytes size");
        return;
    }
    m_streamStartBytes = data;
}

void Kdbx3Reader::setInnerRandomStreamID(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("Invalid random stream id size");
        return;
    }
    quint32 id = Endian::bytesToSizedInt<quint32>(data, KeePass2::BYTEORDER);
    KeePass2::ProtectedStreamAlgo irsAlgo = KeePass2::idToProtectedStreamAlgo(id);
    if (irsAlgo == KeePass2::ProtectedStreamAlgo::InvalidProtectedStreamAlgo ||
        irsAlgo == KeePass2::ProtectedStreamAlgo::ArcFourVariant) {
        raiseError("Invalid inner random stream cipher");
        return;
    }
    m_irsAlgo = irsAlgo;
}
