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

#include "core/Group.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "format/KeePass2RandomStream.h"
#include "format/KdbxXmlReader.h"
#include "streams/HmacBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

Database* Kdbx4Reader::readDatabaseImpl(QIODevice* device, const QByteArray& headerData,
                                        const CompositeKey& key, bool keepDatabase)
{
    Q_ASSERT(m_kdbxVersion == KeePass2::FILE_VERSION_4);

    m_binaryPoolInverse.clear();

    if (hasError()) {
        return nullptr;
    }

    // check if all required headers were present
    if (m_masterSeed.isEmpty()
            || m_encryptionIV.isEmpty()
            || m_db->cipher().isNull()) {
        raiseError(tr("missing database headers"));
        return nullptr;
    }

    if (!m_db->setKey(key, false, false)) {
        raiseError(tr("Unable to calculate master key"));
        return nullptr;
    }

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(m_masterSeed);
    hash.addData(m_db->transformedMasterKey());
    QByteArray finalKey = hash.result();

    QByteArray headerSha256 = device->read(32);
    QByteArray headerHmac = device->read(32);
    if (headerSha256.size() != 32 || headerHmac.size() != 32) {
        raiseError(tr("Invalid header checksum size"));
        return nullptr;
    }
    if (headerSha256 != CryptoHash::hash(headerData, CryptoHash::Sha256)) {
        raiseError(tr("Header SHA256 mismatch"));
        return nullptr;
    }

    QByteArray hmacKey = KeePass2::hmacKey(m_masterSeed, m_db->transformedMasterKey());
    if (headerHmac != CryptoHash::hmac(headerData,
                                       HmacBlockStream::getHmacKey(UINT64_MAX, hmacKey), CryptoHash::Sha256)) {
        raiseError(tr("Wrong key or database file is corrupt. (HMAC mismatch)"));
        return nullptr;
    }
    HmacBlockStream hmacStream(device, hmacKey);
    if (!hmacStream.open(QIODevice::ReadOnly)) {
        raiseError(hmacStream.errorString());
        return nullptr;
    }

    SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
    if (cipher == SymmetricCipher::InvalidAlgorithm) {
        raiseError(tr("Unknown cipher"));
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

    QIODevice* xmlDevice = nullptr;
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

    QBuffer buffer;
    if (saveXml()) {
        m_xmlData = xmlDevice->readAll();
        buffer.setBuffer(&m_xmlData);
        buffer.open(QIODevice::ReadOnly);
        xmlDevice = &buffer;
    }

    Q_ASSERT(xmlDevice);

    KdbxXmlReader xmlReader(KeePass2::FILE_VERSION_4, binaryPool());
    xmlReader.readDatabase(xmlDevice, m_db.data(), &randomStream);

    if (xmlReader.hasError()) {
        raiseError(xmlReader.errorString());
        if (keepDatabase) {
            return m_db.take();
        }
        return nullptr;
    }

    return m_db.take();
}

bool Kdbx4Reader::readHeaderField(StoreDataStream& device)
{
    QByteArray fieldIDArray = device.read(1);
    if (fieldIDArray.size() != 1) {
        raiseError(tr("Invalid header id size"));
        return false;
    }
    char fieldID = fieldIDArray.at(0);

    bool ok;
    auto fieldLen = Endian::readSizedInt<quint32>(&device, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError(tr("Invalid header field length"));
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = device.read(fieldLen);
        if (static_cast<quint32>(fieldData.size()) != fieldLen) {
            raiseError(tr("Invalid header data length"));
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
            raiseError(tr("Failed to open buffer for KDF parameters in header"));
            return false;
        }
        QVariantMap kdfParams = readVariantMap(&bufIoDevice);
        QSharedPointer<Kdf> kdf = KeePass2::kdfFromParameters(kdfParams);
        if (!kdf) {
            raiseError(tr("Unsupported key derivation function (KDF) or invalid parameters"));
            return false;
        }
        m_db->setKdf(kdf);
        break;
    }

    case KeePass2::HeaderFieldID::PublicCustomData: {
        QBuffer variantBuffer;
        variantBuffer.setBuffer(&fieldData);
        variantBuffer.open(QBuffer::ReadOnly);
        QVariantMap data = readVariantMap(&variantBuffer);
        m_db->setPublicCustomData(data);
        break;
    }

    case KeePass2::HeaderFieldID::ProtectedStreamKey:
    case KeePass2::HeaderFieldID::TransformRounds:
    case KeePass2::HeaderFieldID::TransformSeed:
    case KeePass2::HeaderFieldID::StreamStartBytes:
    case KeePass2::HeaderFieldID::InnerRandomStreamID:
        raiseError(tr("Legacy header fields found in KDBX4 file."));
        return false;

    default:
        qWarning("Unknown header field read: id=%d", fieldID);
        break;
    }

    return true;
}

/**
 * Helper method for reading KDBX4 inner header fields.
 *
 * @param device input device
 * @return true if there are more inner header fields
 */
bool Kdbx4Reader::readInnerHeaderField(QIODevice* device)
{
    QByteArray fieldIDArray = device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError(tr("Invalid inner header id size"));
        return false;
    }
    auto fieldID = static_cast<KeePass2::InnerHeaderFieldID>(fieldIDArray.at(0));

    bool ok;
    auto fieldLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError(tr("Invalid inner header field length"));
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = device->read(fieldLen);
        if (static_cast<quint32>(fieldData.size()) != fieldLen) {
            raiseError(tr("Invalid header data length"));
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

    case KeePass2::InnerHeaderFieldID::Binary: {
        if (fieldLen < 1) {
            raiseError(tr("Invalid inner header binary size"));
            return false;
        }
        auto data = fieldData.mid(1);
        if (m_binaryPoolInverse.contains(data)) {
            qWarning("Skipping duplicate binary record");
            break;
        }
        m_binaryPoolInverse.insert(data, QString::number(m_binaryPoolInverse.size()));
        break;
    }
    }

    return true;
}

/**
 * Helper method for reading a serialized variant map.
 *
 * @param device input device
 * @return de-serialized variant map
 */
QVariantMap Kdbx4Reader::readVariantMap(QIODevice* device)
{
    bool ok;
    quint16 version = Endian::readSizedInt<quint16>(device, KeePass2::BYTEORDER, &ok)
            & KeePass2::VARIANTMAP_CRITICAL_MASK;
    quint16 maxVersion = KeePass2::VARIANTMAP_VERSION & KeePass2::VARIANTMAP_CRITICAL_MASK;
    if (!ok || (version > maxVersion)) {
        //: Translation: variant map = data structure for storing meta data
        raiseError(tr("Unsupported KeePass variant map version."));
        return {};
    }

    QVariantMap vm;
    QByteArray fieldTypeArray;
    KeePass2::VariantMapFieldType fieldType = KeePass2::VariantMapFieldType::End;
    while (((fieldTypeArray = device->read(1)).size() == 1)
           && ((fieldType = static_cast<KeePass2::VariantMapFieldType>(fieldTypeArray.at(0)))
               != KeePass2::VariantMapFieldType::End)) {
        auto nameLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
        if (!ok) {
            //: Translation: variant map = data structure for storing meta data
            raiseError(tr("Invalid variant map entry name length"));
            return {};
        }
        QByteArray nameBytes;
        if (nameLen != 0) {
            nameBytes = device->read(nameLen);
            if (static_cast<quint32>(nameBytes.size()) != nameLen) {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map entry name data"));
                return {};
            }
        }
        QString name = QString::fromUtf8(nameBytes);

        auto valueLen = Endian::readSizedInt<quint32>(device, KeePass2::BYTEORDER, &ok);
        if (!ok) {
            //: Translation: variant map = data structure for storing meta data
            raiseError(tr("Invalid variant map entry value length"));
            return {};
        }
        QByteArray valueBytes;
        if (valueLen != 0) {
            valueBytes = device->read(valueLen);
            if (static_cast<quint32>(valueBytes.size()) != valueLen) {
                //: Translation comment: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map entry value data"));
                return {};
            }
        }

        switch (fieldType) {
        case KeePass2::VariantMapFieldType::Bool:
            if (valueLen == 1) {
                vm.insert(name, QVariant(valueBytes.at(0) != 0));
            } else {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map Bool entry value length"));
                return {};
            }
            break;

        case KeePass2::VariantMapFieldType::Int32:
            if (valueLen == 4) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<qint32>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map Int32 entry value length"));
                return {};
            }
            break;

        case KeePass2::VariantMapFieldType::UInt32:
            if (valueLen == 4) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<quint32>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map UInt32 entry value length"));
                return {};
            }
            break;

        case KeePass2::VariantMapFieldType::Int64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<qint64>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map Int64 entry value length"));
                return {};
            }
            break;

        case KeePass2::VariantMapFieldType::UInt64:
            if (valueLen == 8) {
                vm.insert(name, QVariant(Endian::bytesToSizedInt<quint64>(valueBytes, KeePass2::BYTEORDER)));
            } else {
                //: Translation: variant map = data structure for storing meta data
                raiseError(tr("Invalid variant map UInt64 entry value length"));
                return {};
            }
            break;

        case KeePass2::VariantMapFieldType::String:
            vm.insert(name, QVariant(QString::fromUtf8(valueBytes)));
            break;

        case KeePass2::VariantMapFieldType::ByteArray:
            vm.insert(name, QVariant(valueBytes));
            break;

        default:
            //: Translation: variant map = data structure for storing meta data
            raiseError(tr("Invalid variant map entry type"));
            return {};
        }
    }

    if (fieldTypeArray.size() != 1) {
        //: Translation: variant map = data structure for storing meta data
        raiseError(tr("Invalid variant map field type size"));
        return {};
    }

    return vm;
}

/**
 * @return mapping from attachment keys to binary data
 */
QHash<QString, QByteArray> Kdbx4Reader::binaryPool() const
{
    QHash<QString, QByteArray> binaryPool;
    for (auto it = m_binaryPoolInverse.cbegin(); it != m_binaryPoolInverse.cend(); ++it) {
        binaryPool.insert(it.value(), it.key());
    }
    return binaryPool;
}

/**
 * @return mapping from binary data to attachment keys
 */
QHash<QByteArray, QString> Kdbx4Reader::binaryPoolInverse() const
{
    return m_binaryPoolInverse;
}
