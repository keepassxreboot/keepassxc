/*
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

#include "Kdbx4Reader.h"

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
#include "format/Kdbx4XmlReader.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/StoreDataStream.h"
#include "streams/SymmetricCipherStream.h"

Kdbx4Reader::Kdbx4Reader()
    : m_device(nullptr)
    , m_db(nullptr)
{
}

Database* Kdbx4Reader::readDatabase(QIODevice* device, const CompositeKey& key, bool keepDatabase)
{
    QScopedPointer<Database> db(new Database());
    m_db = db.data();
    m_device = device;
    m_error = false;
    m_errorStr.clear();
    m_xmlData.clear();
    m_masterSeed.clear();
    m_encryptionIV.clear();
    m_protectedStreamKey.clear();
    m_binaryPool.clear();

    StoreDataStream headerStream(m_device);
    headerStream.open(QIODevice::ReadOnly);
    QIODevice* headerIo = &headerStream;

    bool ok;

    quint32 signature1 = Endian::readSizedInt<quint32>(headerIo, KeePass2::BYTEORDER, &ok);
    if (!ok || signature1 != KeePass2::SIGNATURE_1) {
        raiseError(tr("Not a KeePass database."));
        return nullptr;
    }

    quint32 signature2 = Endian::readSizedInt<quint32>(headerIo, KeePass2::BYTEORDER, &ok);
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

    quint32 version = Endian::readSizedInt<quint32>(headerIo, KeePass2::BYTEORDER, &ok)
            & KeePass2::FILE_VERSION_CRITICAL_MASK;
    if (!ok || version != (KeePass2::FILE_VERSION_4 & KeePass2::FILE_VERSION_CRITICAL_MASK)) {
        raiseError(tr("Unsupported KeePass KDBX 4 database version."));
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
    HmacBlockStream hmacStream(m_device, hmacKey);
    if (!hmacStream.open(QIODevice::ReadOnly)) {
        raiseError(hmacStream.errorString());
        return nullptr;
    }

    SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
    if (cipher == SymmetricCipher::InvalidAlgorithm) {
        raiseError("Unknown cipher");
        return nullptr;
    }
    SymmetricCipherStream cipherStream(&hmacStream, cipher,
                                       SymmetricCipher::algorithmMode(cipher), SymmetricCipher::Decrypt);
    if (!cipherStream.init(finalKey, m_encryptionIV)) {
        raiseError(cipherStream.errorString());
        return nullptr;
    }
    if (!cipherStream.open(QIODevice::ReadOnly)) {
        raiseError(cipherStream.errorString());
        return nullptr;
    }

    QIODevice* xmlDevice;
    QScopedPointer<QtIOCompressor> ioCompressor;

    if (m_db->compressionAlgo() == Database::CompressionNone) {
        xmlDevice = &cipherStream;
    } else {
        ioCompressor.reset(new QtIOCompressor(&cipherStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::ReadOnly)) {
            raiseError(ioCompressor->errorString());
            return nullptr;
        }
        xmlDevice = ioCompressor.data();
    }


    while (readInnerHeaderField(xmlDevice) && !hasError()) {
    }

    if (hasError()) {
        return nullptr;
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

    Kdbx4XmlReader xmlReader(m_binaryPool);
    xmlReader.readDatabase(xmlDevice, m_db, &randomStream);

    if (xmlReader.hasError()) {
        raiseError(xmlReader.errorString());
        if (keepDatabase) {
            return db.take();
        }
        return nullptr;
    }

    return db.take();
}

bool Kdbx4Reader::readHeaderField(QIODevice* device)
{
    QByteArray fieldIDArray = device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid header id size");
        return false;
    }
    char fieldID = fieldIDArray.at(0);

    bool ok;
    auto fieldLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
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

    switch (static_cast<KeePass2::HeaderFieldID>(fieldID)) {
    case KeePass2::HeaderFieldID::EndOfHeader:
        return false;

    case KeePass2::HeaderFieldID::CipherID:
        setCipher(fieldData);
        break;

    case KeePass2::HeaderFieldID::CompressionFlags:
        setCompressionFlags(fieldData);
        break;

    case KeePass2::HeaderFieldID::MasterSeed:
        setMasterSeed(fieldData);
        break;

    case KeePass2::HeaderFieldID::EncryptionIV:
        setEncryptionIV(fieldData);
        break;

    case KeePass2::HeaderFieldID::KdfParameters: {
        QBuffer bufIoDevice(&fieldData);
        if (!bufIoDevice.open(QIODevice::ReadOnly)) {
            raiseError("Failed to open buffer for KDF parameters in header");
            return false;
        }
        QVariantMap kdfParams = readVariantMap(&bufIoDevice);
        QSharedPointer<Kdf> kdf = KeePass2::kdfFromParameters(kdfParams);
        if (!kdf) {
            raiseError("Invalid KDF parameters");
            return false;
        }
        m_db->setKdf(kdf);
        break;
    }

    case KeePass2::HeaderFieldID::PublicCustomData:
        m_db->setPublicCustomData(fieldData);
        break;

    case KeePass2::HeaderFieldID::ProtectedStreamKey:
    case KeePass2::HeaderFieldID::TransformRounds:
    case KeePass2::HeaderFieldID::TransformSeed:
    case KeePass2::HeaderFieldID::StreamStartBytes:
    case KeePass2::HeaderFieldID::InnerRandomStreamID:
        raiseError("Legacy header fields found in KDBX4 file.");
        return false;

    default:
        qWarning("Unknown header field read: id=%d", fieldID);
        break;
    }

    return true;
}

bool Kdbx4Reader::readInnerHeaderField(QIODevice* device)
{
    QByteArray fieldIDArray = device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid inner header id size");
        return false;
    }
    KeePass2::InnerHeaderFieldID fieldID = static_cast<KeePass2::InnerHeaderFieldID>(fieldIDArray.at(0));

    bool ok;
    quint32 fieldLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
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
        m_binaryPool.insert(QString::number(m_binaryPool.size()), fieldData.mid(1));
        break;

    default:
        qWarning("Unknown inner header field read: id=%hhu", static_cast<quint8>(fieldID));
        break;
    }

    return true;
}

QVariantMap Kdbx4Reader::readVariantMap(QIODevice* device)
{
    bool ok;
    quint16 version = Endian::readSizedInt<quint16>(device, KeePass2::BYTEORDER, &ok)
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
        quint32 nameLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
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

        quint32 valueLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
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
                vm.insert(name, QVariant(Endian::bytesToSizedInt<qint32>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map Int32 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::UInt32:
            if (valueLen == 4) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<quint32>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map UInt32 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::Int64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<qint64>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                raiseError("Invalid variant map Int64 entry value length");
                return QVariantMap();
            }
            break;
        case KeePass2::VariantMapFieldType::UInt64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<quint64>(valueBytes, KeePass2::BYTEORDER)));
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

void Kdbx4Reader::setCipher(const QByteArray& data)
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

void Kdbx4Reader::setCompressionFlags(const QByteArray& data)
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

void Kdbx4Reader::setMasterSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("Invalid master seed size");
        return;
    }
    m_masterSeed = data;
}

void Kdbx4Reader::setEncryptionIV(const QByteArray& data)
{
    m_encryptionIV = data;
}

void Kdbx4Reader::setProtectedStreamKey(const QByteArray& data)
{
    m_protectedStreamKey = data;
}

void Kdbx4Reader::setInnerRandomStreamID(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("Invalid random stream id size");
        return;
    }
    auto id = Endian::bytesToSizedInt<quint32>(data, KeePass2::BYTEORDER);
    KeePass2::ProtectedStreamAlgo irsAlgo = KeePass2::idToProtectedStreamAlgo(id);
    if (irsAlgo == KeePass2::ProtectedStreamAlgo::InvalidProtectedStreamAlgo || irsAlgo == KeePass2::ProtectedStreamAlgo::ArcFourVariant) {
        raiseError("Invalid inner random stream cipher");
        return;
    }
    m_irsAlgo = irsAlgo;
}

QHash<QString, QByteArray> Kdbx4Reader::binaryPool()
{
    return m_binaryPool;
}
