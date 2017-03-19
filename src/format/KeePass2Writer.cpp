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

#include "KeePass2Writer.h"

#include <QBuffer>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>

#include "streams/HmacBlockStream.h"
#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass2RandomStream.h"
#include "format/KeePass2XmlWriter.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

#define CHECK_RETURN_FALSE(x) if (!(x)) return false;

KeePass2Writer::KeePass2Writer()
    : m_device(0)
    , m_error(false)
{
}

bool KeePass2Writer::writeDatabase(QIODevice* device, Database* db)
{
    m_error = false;
    m_errorStr.clear();

    bool kdbx3 = db->kdf()->type() == Kdf::Type::AES;
    SymmetricCipher::Algorithm algo = SymmetricCipher::cipherToAlgorithm(db->cipher());
    if (algo == SymmetricCipher::InvalidAlgorithm) {
        raiseError("Invalid symmetric cipher algorithm.");
        return false;
    }
    int ivSize = SymmetricCipher::algorithmIvSize(algo);
    if (ivSize < 0) {
        raiseError("Invalid symmetric cipher IV size.");
        return false;
    }
    QByteArray masterSeed = randomGen()->randomArray(32);
    QByteArray encryptionIV = randomGen()->randomArray(ivSize);
    QByteArray protectedStreamKey = randomGen()->randomArray(kdbx3 ? 32 : 64);
    QByteArray startBytes;
    QByteArray endOfHeader = "\r\n\r\n";


    if (db->challengeMasterSeed(masterSeed) == false) {
        raiseError("Unable to issue challenge-response.");
        return false;
    }

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(masterSeed);
    hash.addData(db->challengeResponseKey());
    Q_ASSERT(!db->transformedMasterKey().isEmpty());
    hash.addData(db->transformedMasterKey());
    QByteArray finalKey = hash.result();

    QByteArray headerData;
    {
        QBuffer header;
        header.open(QIODevice::WriteOnly);
        m_device = &header;
        CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(KeePass2::SIGNATURE_1, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(KeePass2::SIGNATURE_2, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(kdbx3 ?
                                                          KeePass2::FILE_VERSION_3 :
                                                          KeePass2::FILE_VERSION, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::CipherID, db->cipher().toByteArray(), kdbx3));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::CompressionFlags,
                                            Endian::int32ToBytes(db->compressionAlgo(),
                                                                 KeePass2::BYTEORDER), kdbx3));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::MasterSeed, masterSeed, kdbx3));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::EncryptionIV, encryptionIV, kdbx3));
        if (kdbx3) {
            AesKdf* kdf = static_cast<AesKdf*>(db->kdf());
            startBytes = randomGen()->randomArray(32);
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::TransformSeed, kdf->seed(), kdbx3));
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::TransformRounds,
                                          Endian::int64ToBytes(kdf->rounds(),
                                                               KeePass2::BYTEORDER), kdbx3));
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::ProtectedStreamKey, protectedStreamKey, kdbx3));
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::StreamStartBytes, startBytes, kdbx3));
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::InnerRandomStreamID,
                                          Endian::int32ToBytes(KeePass2::Salsa20,
                                                               KeePass2::BYTEORDER), kdbx3));

        } else {
            QVariantMap kdfParams;
            if (!KeePass2::kdfToParams(*db->kdf(), kdfParams)) {
                raiseError("Failed to convert KDF parameters to variant map");
                return false;
            }
            QByteArray kdfParamBytes;
            if (!serializeVariantMap(kdfParams, kdfParamBytes)) {
                raiseError("Failed to serialise KDF parameters variant map");
                return false;
            }
            QByteArray publicCustomData = db->publicCustomData();
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::KdfParameters, kdfParamBytes, kdbx3));
            if (!publicCustomData.isEmpty()) {
                CHECK_RETURN_FALSE(writeHeaderField(KeePass2::PublicCustomData, publicCustomData, kdbx3));
            }
        }
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::EndOfHeader, endOfHeader, kdbx3));
        header.close();
        m_device = device;
        headerData = header.data();
    }
    CHECK_RETURN_FALSE(writeData(headerData));
    QByteArray headerHash = CryptoHash::hash(headerData, CryptoHash::Sha256);

    QScopedPointer<QIODevice> firstLayer, secondLayer;
    if (kdbx3) {
        SymmetricCipherStream* cipherStream = new SymmetricCipherStream(device, algo,
                                                                        SymmetricCipher::algorithmMode(algo),
                                                                        SymmetricCipher::Encrypt);
        if (!cipherStream->init(finalKey, encryptionIV)) {
            raiseError(cipherStream->errorString());
            return false;
        }
        if (!cipherStream->open(QIODevice::WriteOnly)) {
            raiseError(cipherStream->errorString());
            return false;
        }
        firstLayer.reset(static_cast<QIODevice*>(cipherStream));
        m_device = cipherStream;
        CHECK_RETURN_FALSE(writeData(startBytes));

        HashedBlockStream* hashedStream = new HashedBlockStream(cipherStream);
        if (!hashedStream->open(QIODevice::WriteOnly)) {
            raiseError(hashedStream->errorString());
            return false;
        }
        secondLayer.reset(static_cast<QIODevice*>(hashedStream));
    } else {
        QByteArray hmacKey = KeePass2::hmacKey(masterSeed, db->transformedMasterKey());
        QByteArray headerHmac = CryptoHash::hmac(headerData, HmacBlockStream::getHmacKey(UINT64_MAX, hmacKey),
                                                 CryptoHash::Sha256);
        CHECK_RETURN_FALSE(writeData(headerHash));
        CHECK_RETURN_FALSE(writeData(headerHmac));

        HmacBlockStream* hmacStream = new HmacBlockStream(device, hmacKey);
        if (!hmacStream->open(QIODevice::WriteOnly)) {
            raiseError(hmacStream->errorString());
            return false;
        }
        firstLayer.reset(static_cast<QIODevice*>(hmacStream));

        SymmetricCipherStream* cipherStream = new SymmetricCipherStream(hmacStream, algo,
                                                                        SymmetricCipher::algorithmMode(algo),
                                                                        SymmetricCipher::Encrypt);
        if (!cipherStream->init(finalKey, encryptionIV)) {
            raiseError(cipherStream->errorString());
            return false;
        }
        if (!cipherStream->open(QIODevice::WriteOnly)) {
            raiseError(cipherStream->errorString());
            return false;
        }
        secondLayer.reset(static_cast<QIODevice*>(cipherStream));
    }

    QScopedPointer<QtIOCompressor> ioCompressor;
    if (db->compressionAlgo() == Database::CompressionNone) {
        m_device = secondLayer.data();
    } else {
        ioCompressor.reset(new QtIOCompressor(secondLayer.data()));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::WriteOnly)) {
            raiseError(ioCompressor->errorString());
            return false;
        }
        m_device = ioCompressor.data();
    }

    QHash<QByteArray, int> idMap;
    if (!kdbx3) {
        CHECK_RETURN_FALSE(writeInnerHeaderField(KeePass2::InnerHeaderFieldID::InnerRandomStreamID,
                                                 Endian::int32ToBytes(KeePass2::ChaCha20, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeInnerHeaderField(KeePass2::InnerHeaderFieldID::InnerRandomStreamKey,
                                                 protectedStreamKey));
        const QList<Entry*> allEntries = db->rootGroup()->entriesRecursive(true);
        int nextId = 0;

        for (Entry* entry : allEntries) {
            const QList<QString> attachmentKeys = entry->attachments()->keys();
            for (const QString& key : attachmentKeys) {
                QByteArray data = entry->attachments()->value(key);
                if (!idMap.contains(data)) {
                    CHECK_RETURN_FALSE(writeBinary(data));
                    idMap.insert(data, nextId++);
                }
            }
        }
        CHECK_RETURN_FALSE(writeInnerHeaderField(KeePass2::InnerHeaderFieldID::End, QByteArray()));
    }

    KeePass2RandomStream randomStream(kdbx3 ? KeePass2::Salsa20 : KeePass2::ChaCha20);
    if (!randomStream.init(protectedStreamKey)) {
        raiseError(randomStream.errorString());
        return false;
    }

    KeePass2XmlWriter xmlWriter(kdbx3 ? KeePass2::FILE_VERSION_3 : KeePass2::FILE_VERSION_4, idMap);
    xmlWriter.writeDatabase(m_device, db, &randomStream, headerHash);

    // Explicitly close/reset streams so they are flushed and we can detect
    // errors. QIODevice::close() resets errorString() etc.
    if (ioCompressor) {
        ioCompressor->close();
    }
    if (!secondLayer->reset()) {
        raiseError(secondLayer->errorString());
        return false;
    }
    if (!firstLayer->reset()) {
        raiseError(firstLayer->errorString());
        return false;
    }

    if (xmlWriter.hasError()) {
        raiseError(xmlWriter.errorString());
        return false;
    }

    return true;
}

bool KeePass2Writer::writeData(const QByteArray& data)
{
    if (m_device->write(data) != data.size()) {
        raiseError(m_device->errorString());
        return false;
    }
    else {
        return true;
    }
}

bool KeePass2Writer::writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data, bool kdbx3)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = fieldId;
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    if (kdbx3) {
        CHECK_RETURN_FALSE(writeData(Endian::uint16ToBytes(static_cast<quint16>(data.size()), KeePass2::BYTEORDER)));
    } else {
        CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(static_cast<quint32>(data.size()), KeePass2::BYTEORDER)));
    }
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool KeePass2Writer::writeInnerHeaderField(KeePass2::InnerHeaderFieldID fieldId, const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(fieldId);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(static_cast<quint32>(data.size()), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool KeePass2Writer::writeBinary(const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(KeePass2::InnerHeaderFieldID::Binary);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::uint32ToBytes(static_cast<quint32>(data.size() + 1), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(QByteArray(1, '\1')));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool KeePass2Writer::serializeVariantMap(const QVariantMap& p, QByteArray& o)
{
    QBuffer buf(&o);
    buf.open(QIODevice::WriteOnly);
    CHECK_RETURN_FALSE(buf.write(Endian::uint16ToBytes(KeePass2::VARIANTMAP_VERSION, KeePass2::BYTEORDER)) == 2);

    bool ok;
    QList<QString> keys = p.keys();
    for (int i = 0; i < keys.size(); ++i) {
        QString k = keys.at(i);
        KeePass2::VariantMapFieldType fieldType;
        QByteArray data;
        QVariant v = p.value(k);
        switch (static_cast<QMetaType::Type>(v.type())) {
            case QMetaType::Type::Int:
                fieldType = KeePass2::VariantMapFieldType::Int32;
                data = Endian::int32ToBytes(v.toInt(&ok), KeePass2::BYTEORDER);
                CHECK_RETURN_FALSE(ok);
                break;
            case QMetaType::Type::UInt:
                fieldType = KeePass2::VariantMapFieldType::UInt32;
                data = Endian::uint32ToBytes(v.toUInt(&ok), KeePass2::BYTEORDER);
                CHECK_RETURN_FALSE(ok);
                break;
            case QMetaType::Type::LongLong:
                fieldType = KeePass2::VariantMapFieldType::Int64;
                data = Endian::int64ToBytes(v.toLongLong(&ok), KeePass2::BYTEORDER);
                CHECK_RETURN_FALSE(ok);
                break;
            case QMetaType::Type::ULongLong:
                fieldType = KeePass2::VariantMapFieldType::UInt64;
                data = Endian::uint64ToBytes(v.toULongLong(&ok), KeePass2::BYTEORDER);
                CHECK_RETURN_FALSE(ok);
                break;
            case QMetaType::Type::QString:
                fieldType = KeePass2::VariantMapFieldType::String;
                data = v.toString().toUtf8();
                break;
            case QMetaType::Type::Bool:
                fieldType = KeePass2::VariantMapFieldType::Bool;
                data = QByteArray(1, (v.toBool() ? '\1' : '\0'));
                break;
            case QMetaType::Type::QByteArray:
                fieldType = KeePass2::VariantMapFieldType::ByteArray;
                data = v.toByteArray();
                break;
            default:
                qWarning("Unknown object type %d in QVariantMap", v.type());
                return false;
        }
        QByteArray typeBytes;
        typeBytes[0] = static_cast<char>(fieldType);
        QByteArray nameBytes = k.toUtf8();
        QByteArray nameLenBytes = Endian::int32ToBytes(nameBytes.size(), KeePass2::BYTEORDER);
        QByteArray dataLenBytes = Endian::int32ToBytes(data.size(), KeePass2::BYTEORDER);

        CHECK_RETURN_FALSE(buf.write(typeBytes) == 1);
        CHECK_RETURN_FALSE(buf.write(nameLenBytes) == 4);
        CHECK_RETURN_FALSE(buf.write(nameBytes) == nameBytes.size());
        CHECK_RETURN_FALSE(buf.write(dataLenBytes) == 4);
        CHECK_RETURN_FALSE(buf.write(data) == data.size());
    }

    QByteArray endBytes;
    endBytes[0] = static_cast<char>(KeePass2::VariantMapFieldType::End);
    CHECK_RETURN_FALSE(buf.write(endBytes) == 1);
    return true;
}

void KeePass2Writer::writeDatabase(const QString& filename, Database* db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        raiseError(file.errorString());
        return;
    }
    writeDatabase(&file, db);
}

bool KeePass2Writer::hasError()
{
    return m_error;
}

QString KeePass2Writer::errorString()
{
    return m_errorStr;
}

void KeePass2Writer::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}
