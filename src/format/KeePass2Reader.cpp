/*
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

#include "KeePass2Reader.h"

#include <QBuffer>
#include <QFile>

#include "crypto/kdf/AesKdf.h"
#include "streams/HmacBlockStream.h"
#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "format/KeePass1.h"
#include "format/KeePass2.h"
#include "format/KeePass2RandomStream.h"
#include "format/KeePass2XmlReader.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/StoreDataStream.h"
#include "streams/SymmetricCipherStream.h"

KeePass2Reader::KeePass2Reader()
    : m_device(nullptr)
    , m_error(false)
    , m_saveXml(false)
    , m_db(nullptr)
    , m_irsAlgo(KeePass2::InvalidProtectedStreamAlgo)
{
}

Database* KeePass2Reader::readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase)
{
    QScopedPointer<Database> db(new Database());
    m_db = db.data();
    m_device = device;
    m_error = false;
    m_errorStr.clear();
    m_xmlData.clear();
    m_masterSeed.clear();
    m_encryptionIV.clear();
    m_streamStartBytes.clear();
    m_protectedStreamKey.clear();

    StoreDataStream headerStream(m_device);
    headerStream.open(QIODevice::ReadOnly);
    QIODevice* headerIo = &headerStream;

    bool ok;

    quint32 signature1 = Endian::readUInt32(headerIo, KeePass2::BYTEORDER, &ok);
    if (!ok || signature1 != KeePass2::SIGNATURE_1) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    quint32 signature2 = Endian::readUInt32(headerIo, KeePass2::BYTEORDER, &ok);
    if (ok && signature2 == KeePass1::SIGNATURE_2) {
        raiseError(tr("The selected file is an old KeePass 1 database (.kdb).\n\n"
                      "You can import it by clicking on Database > 'Import KeePass 1 database'.\n"
                      "This is a one-way migration. You won't be able to open the imported "
                      "database with the old KeePassX 0.4 version."));
        return nullptr;
    }
    else if (!ok || signature2 != KeePass2::SIGNATURE_2) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    m_version = Endian::readUInt32(headerIo, KeePass2::BYTEORDER, &ok)
            & KeePass2::FILE_VERSION_CRITICAL_MASK;
    quint32 maxVersion = KeePass2::FILE_VERSION & KeePass2::FILE_VERSION_CRITICAL_MASK;
    if (!ok || (m_version < KeePass2::FILE_VERSION_MIN) || (m_version > maxVersion)) {
        raiseError(tr("Unsupported KeePass database version."));
        return nullptr;
    }

    while (readHeaderField(headerIo) && !hasError()) {
    }

    headerStream.close();

    if (hasError()) {
        return nullptr;
    }

    // check if all required headers were present
    if (m_masterSeed.isEmpty()
            || m_encryptionIV.isEmpty()
            || (m_streamStartBytes.isEmpty() && m_version < KeePass2::FILE_VERSION_4)
            || (m_protectedStreamKey.isEmpty() && m_version < KeePass2::FILE_VERSION_4) // in inner header for kdbx 4
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

    QScopedPointer<QIODevice> preCleartextStream;
    QScopedPointer<QIODevice> cleartextStream;
    if (m_version < KeePass2::FILE_VERSION_4) {
        SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
        if (cipher == SymmetricCipher::InvalidAlgorithm) {
            raiseError("Unknown cipher");
            return nullptr;
        }
        SymmetricCipherStream* cipherStream = new SymmetricCipherStream(m_device, cipher,
                                                                        SymmetricCipher::algorithmMode(cipher), SymmetricCipher::Decrypt);
        if (!cipherStream->init(finalKey, m_encryptionIV)) {
            raiseError(cipherStream->errorString());
            return nullptr;
        }
        if (!cipherStream->open(QIODevice::ReadOnly)) {
            raiseError(cipherStream->errorString());
            return nullptr;
        }

        QByteArray realStart = cipherStream->read(32);

        if (realStart != m_streamStartBytes) {
            raiseError(tr("Wrong key or database file is corrupt."));
            return nullptr;
        }

        HashedBlockStream* hashedStream = new HashedBlockStream(cipherStream);
        if (!hashedStream->open(QIODevice::ReadOnly)) {
            raiseError(hashedStream->errorString());
            return nullptr;
        }

        preCleartextStream.reset(cipherStream);
        cleartextStream.reset(hashedStream);
    } else {
        QByteArray headerSha256 = m_device->read(32);
        QByteArray headerHmac = m_device->read(32);
        if (headerSha256.size() != 32 || headerHmac.size() != 32) {
            raiseError("Invalid header checksum size");
            return nullptr;
        }
        if (headerSha256 != CryptoHash::hash(headerStream.storedData(), CryptoHash::Sha256)) {
            raiseError("Header SHA256 mismatch");
            return nullptr;
        }

        QByteArray hmacKey = KeePass2::hmacKey(m_masterSeed, m_db->transformedMasterKey());
        if (headerHmac != CryptoHash::hmac(headerStream.storedData(),
                                           HmacBlockStream::getHmacKey(UINT64_MAX, hmacKey), CryptoHash::Sha256)) {
            raiseError(tr("Wrong key or database file is corrupt. (HMAC mismatch)"));
            return nullptr;
        }
        HmacBlockStream* hmacStream = new HmacBlockStream(m_device, hmacKey);
        if (!hmacStream->open(QIODevice::ReadOnly)) {
            raiseError(hmacStream->errorString());
            return nullptr;
        }

        SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
        if (cipher == SymmetricCipher::InvalidAlgorithm) {
            raiseError("Unknown cipher");
            return nullptr;
        }
        SymmetricCipherStream* cipherStream = new SymmetricCipherStream(hmacStream, cipher,
                                                 SymmetricCipher::algorithmMode(cipher), SymmetricCipher::Decrypt);
        if (!cipherStream->init(finalKey, m_encryptionIV)) {
            raiseError(cipherStream->errorString());
            return nullptr;
        }
        if (!cipherStream->open(QIODevice::ReadOnly)) {
            raiseError(cipherStream->errorString());
            return nullptr;
        }

        preCleartextStream.reset(hmacStream);
        cleartextStream.reset(cipherStream);
    }

    QIODevice* xmlDevice;
    QScopedPointer<QtIOCompressor> ioCompressor;

    if (m_db->compressionAlgo() == Database::CompressionNone) {
        xmlDevice = cleartextStream.data();
    }
    else {
        ioCompressor.reset(new QtIOCompressor(cleartextStream.data()));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::ReadOnly)) {
            raiseError(ioCompressor->errorString());
            return nullptr;
        }
        xmlDevice = ioCompressor.data();
    }

    if (m_version >= KeePass2::FILE_VERSION_4) {
        while (readInnerHeaderField(xmlDevice) && !hasError()) {
        }
        if (hasError()) {
            return nullptr;
        }
    }

    KeePass2RandomStream randomStream(m_irsAlgo);
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

    QHash<QString, QByteArray> binaryPool;
    for (int i = 0; i < m_binaryPool.size(); ++i) {
        binaryPool.insert(QString::number(i), m_binaryPool.at(i));
    }

    KeePass2XmlReader xmlReader(binaryPool);
    xmlReader.readDatabase(xmlDevice, m_db, &randomStream);

    if (xmlReader.hasError()) {
        raiseError(xmlReader.errorString());
        if (keepDatabase) {
            return db.take();
        }
        else {
            return nullptr;
        }
    }

    Q_ASSERT(m_version < 0x00030001 || m_version >= KeePass2::FILE_VERSION_4 || !xmlReader.headerHash().isEmpty());

    if (!xmlReader.headerHash().isEmpty()) {
        QByteArray headerHash = CryptoHash::hash(headerStream.storedData(), CryptoHash::Sha256);
        if (headerHash != xmlReader.headerHash()) {
            raiseError("Header doesn't match hash");
            return Q_NULLPTR;
        }
    }

    return db.take();
}

Database* KeePass2Reader::readDatabase(const QString& filename, const CompositeKey& key)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        raiseError(file.errorString());
        return nullptr;
    }

    QScopedPointer<Database> db(readDatabase(&file, key));

    if (file.error() != QFile::NoError) {
        raiseError(file.errorString());
        return nullptr;
    }

    return db.take();
}

bool KeePass2Reader::hasError()
{
    return m_error;
}

QString KeePass2Reader::errorString()
{
    return m_errorStr;
}

void KeePass2Reader::setSaveXml(bool save)
{
    m_saveXml = save;
}

QByteArray KeePass2Reader::xmlData()
{
    return m_xmlData;
}

QByteArray KeePass2Reader::streamKey()
{
    return m_protectedStreamKey;
}

void KeePass2Reader::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}

bool KeePass2Reader::readHeaderField(QIODevice* device)
{
    QByteArray fieldIDArray = device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid header id size");
        return false;
    }
    quint8 fieldID = fieldIDArray.at(0);

    bool ok;
    quint32 fieldLen;
    if (m_version < KeePass2::FILE_VERSION_4) {
        fieldLen = Endian::readUInt16(device, KeePass2::BYTEORDER, &ok);
    } else {
        fieldLen = Endian::readUInt32(device, KeePass2::BYTEORDER, &ok);
    }
    if (!ok) {
        raiseError("Invalid header field length");
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = device->read(fieldLen);
        if (static_cast<quint32>(fieldData.size()) != fieldLen) {
            raiseError("Invalid header data length");
            return false;
        }
    }

    switch (fieldID) {
    case KeePass2::EndOfHeader:
        return false;

    case KeePass2::CipherID:
        setCipher(fieldData);
        break;

    case KeePass2::CompressionFlags:
        setCompressionFlags(fieldData);
        break;

    case KeePass2::MasterSeed:
        setMasterSeed(fieldData);
        break;

    case KeePass2::TransformSeed:
        Q_ASSERT(m_version < KeePass2::FILE_VERSION_4);
        setAesTransformSeed(fieldData);
        break;

    case KeePass2::TransformRounds:
        Q_ASSERT(m_version < KeePass2::FILE_VERSION_4);
        setAesTransformRounds(fieldData);
        break;

    case KeePass2::EncryptionIV:
        setEncryptionIV(fieldData);
        break;

    case KeePass2::ProtectedStreamKey:
        Q_ASSERT(m_version < KeePass2::FILE_VERSION_4);
        setProtectedStreamKey(fieldData);
        break;

    case KeePass2::StreamStartBytes:
        Q_ASSERT(m_version < KeePass2::FILE_VERSION_4);
        setStreamStartBytes(fieldData);
        break;

    case KeePass2::InnerRandomStreamID:
        Q_ASSERT(m_version < KeePass2::FILE_VERSION_4);
        setInnerRandomStreamID(fieldData);
        break;

    case KeePass2::KdfParameters: {
        QBuffer bufIoDevice(&fieldData);
        if (!bufIoDevice.open(QIODevice::ReadOnly)) {
            raiseError("Failed to open buffer for KDF parameters in header");
            return false;
        }
        QVariantMap kdfParams = readVariantMap(&bufIoDevice);
        Kdf* kdf = KeePass2::paramsToKdf(kdfParams);
        if (kdf == nullptr) {
            raiseError("Invalid KDF parameters");
            return false;
        }
        m_db->setKdf(kdf);
        break;
    }

    case KeePass2::PublicCustomData:
        m_db->setPublicCustomData(fieldData);
        break;

    default:
        qWarning("Unknown header field read: id=%d", fieldID);
        break;
    }

    return true;
}

bool KeePass2Reader::readInnerHeaderField(QIODevice* device)
{
    QByteArray fieldIDArray = device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid inner header id size");
        return false;
    }
    KeePass2::InnerHeaderFieldID fieldID = static_cast<KeePass2::InnerHeaderFieldID>(fieldIDArray.at(0));

    bool ok;
    quint32 fieldLen = Endian::readUInt32(device, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError("Invalid inner header field length");
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = device->read(fieldLen);
        if (static_cast<quint32>(fieldData.size()) != fieldLen) {
            raiseError("Invalid header data length");
            return false;
        }
    }

    switch (fieldID) {
    case KeePass2::InnerHeaderFieldID::End:
        return false;

    case KeePass2::InnerHeaderFieldID::InnerRandomStreamID:
        setInnerRandomStreamID(fieldData);
        break;

    case KeePass2::InnerHeaderFieldID::InnerRandomStreamKey:
        setProtectedStreamKey(fieldData);
        break;

    case KeePass2::InnerHeaderFieldID::Binary:
        if (fieldLen < 1) {
            raiseError("Invalid inner header binary size");
            return false;
        }
        m_binaryPool.append(fieldData.mid(1));
        break;

    default:
        qWarning("Unknown inner header field read: id=%hhu", static_cast<quint8>(fieldID));
        break;
    }

    return true;
}

QVariantMap KeePass2Reader::readVariantMap(QIODevice* device)
{
    bool ok;
    quint16 version = Endian::readUInt16(device, KeePass2::BYTEORDER, &ok)
            & KeePass2::VARIANTMAP_CRITICAL_MASK;
    quint16 maxVersion = KeePass2::VARIANTMAP_VERSION & KeePass2::VARIANTMAP_CRITICAL_MASK;
    if (!ok || (version > maxVersion)) {
        raiseError(tr("Unsupported KeePass variant map version."));
        return QVariantMap();
    }

    QVariantMap vm;
    QByteArray fieldTypeArray;
    KeePass2::VariantMapFieldType fieldType;
    while (((fieldTypeArray = device->read(1)).size() == 1)
           && ((fieldType = static_cast<KeePass2::VariantMapFieldType>(fieldTypeArray.at(0)))
               != KeePass2::VariantMapFieldType::End)) {
        quint32 nameLen = Endian::readUInt32(device, KeePass2::BYTEORDER, &ok);
        if (!ok) {
            raiseError("Invalid variant map entry name length");
            return QVariantMap();
        }
        QByteArray nameBytes;
        if (nameLen != 0) {
            nameBytes = device->read(nameLen);
            if (static_cast<quint32>(nameBytes.size()) != nameLen) {
                raiseError("Invalid variant map entry name data");
                return QVariantMap();
            }
        }
        QString name = QString::fromUtf8(nameBytes);

        quint32 valueLen = Endian::readUInt32(device, KeePass2::BYTEORDER, &ok);
        if (!ok) {
            raiseError("Invalid variant map entry value length");
            return QVariantMap();
        }
        QByteArray valueBytes;
        if (valueLen != 0) {
            valueBytes = device->read(valueLen);
            if (static_cast<quint32>(valueBytes.size()) != valueLen) {
                raiseError("Invalid variant map entry value data");
                return QVariantMap();
            }
        }

        switch (fieldType) {
        case KeePass2::VariantMapFieldType::Bool:
            if (valueLen == 1) {
                vm.insert(name, QVariant(valueBytes.at(0) != 0));
            } else {
                raiseError("Invalid variant map Bool entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::Int32:
            if (valueLen == 4) {
                vm.insert(name, QVariant(Endian::bytesToInt32(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map Int32 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::UInt32:
            if (valueLen == 4) {
                vm.insert(name, QVariant(Endian::bytesToUInt32(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map UInt32 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::Int64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToInt64(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map Int64 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::UInt64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToUInt64(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map UInt64 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::String:
            vm.insert(name, QVariant(QString::fromUtf8(valueBytes)));
            break;
        case KeePass2::VariantMapFieldType::ByteArray:
            vm.insert(name, QVariant(valueBytes));
            break;
        default:
            raiseError("Invalid variant map entry type");
            return QVariantMap();
        }
    }

    if (fieldTypeArray.size() != 1) {
        raiseError("Invalid variant map field type size");
        return QVariantMap();
    }

    return vm;
}

void KeePass2Reader::setCipher(const QByteArray& data)
{
    if (data.size() != Uuid::Length) {
        raiseError("Invalid cipher uuid length");
    } else {
        Uuid uuid(data);

        if (SymmetricCipher::cipherToAlgorithm(uuid) == SymmetricCipher::InvalidAlgorithm) {
            raiseError("Unsupported cipher");
        } else {
            m_db->setCipher(uuid);
        }
    }
}

void KeePass2Reader::setCompressionFlags(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("Invalid compression flags length");
    } else {
        quint32 id = Endian::bytesToUInt32(data, KeePass2::BYTEORDER);

        if (id > Database::CompressionAlgorithmMax) {
            raiseError("Unsupported compression algorithm");
        } else {
            m_db->setCompressionAlgo(static_cast<Database::CompressionAlgorithm>(id));
        }
    }
}

void KeePass2Reader::setMasterSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid master seed size");
    } else {
        m_masterSeed = data;
    }
}

void KeePass2Reader::setAesTransformSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid transform seed size");
    } else {
        AesKdf* aesKdf;
        if (m_db->kdf()->type() == Kdf::Type::AES) {
            aesKdf = static_cast<AesKdf*>(m_db->kdf());
        } else {
            aesKdf = new AesKdf();
            m_db->setKdf(aesKdf);
        }

        aesKdf->setSeed(data);
    }
}

void KeePass2Reader::setAesTransformRounds(const QByteArray& data)
{
    if (data.size() != 8) {
        raiseError("Invalid transform rounds size");
    } else {
        quint64 rounds = Endian::bytesToUInt64(data, KeePass2::BYTEORDER);

        AesKdf* aesKdf;
        if (m_db->kdf()->type() == Kdf::Type::AES) {
            aesKdf = static_cast<AesKdf*>(m_db->kdf());
        } else {
            aesKdf = new AesKdf();
            m_db->setKdf(aesKdf);
        }

        aesKdf->setRounds(rounds);
    }
}

void KeePass2Reader::setEncryptionIV(const QByteArray& data)
{
    m_encryptionIV = data;
}

void KeePass2Reader::setProtectedStreamKey(const QByteArray& data)
{
    m_protectedStreamKey = data;
}

void KeePass2Reader::setStreamStartBytes(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid start bytes size");
    } else {
        m_streamStartBytes = data;
    }
}

void KeePass2Reader::setInnerRandomStreamID(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("Invalid random stream id size");
    } else {
        quint32 id = Endian::bytesToUInt32(data, KeePass2::BYTEORDER);
        switch (id) {
            case static_cast<quint32>(KeePass2::Salsa20):
                m_irsAlgo = KeePass2::Salsa20;
                break;
            case static_cast<quint32>(KeePass2::ChaCha20):
                m_irsAlgo = KeePass2::ChaCha20;
                break;
            default:
                raiseError("Invalid inner random stream cipher");
                break;
        }
    }
}

KeePass2::ProtectedStreamAlgo KeePass2Reader::protectedStreamAlgo() const {
    return m_irsAlgo;
}
