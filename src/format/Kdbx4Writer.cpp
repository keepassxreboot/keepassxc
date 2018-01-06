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

#include "Kdbx4Writer.h"

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
#include "format/KeePass2RandomStream.h"
#include "format/Kdbx4XmlWriter.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

Kdbx4Writer::Kdbx4Writer()
    : m_device(nullptr)
{
}

bool Kdbx4Writer::writeDatabase(QIODevice* device, Database* db)
{
    m_error = false;
    m_errorStr.clear();

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
    QByteArray protectedStreamKey = randomGen()->randomArray(64);
    QByteArray startBytes;
    QByteArray endOfHeader = "\r\n\r\n";

    if (!db->challengeMasterSeed(masterSeed)) {
        raiseError(tr("Unable to issue challenge-response."));
        return false;
    }

    if (!db->setKey(db->key(), false, true)) {
        raiseError(tr("Unable to calculate master key"));
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
        CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(KeePass2::SIGNATURE_1, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(KeePass2::SIGNATURE_2, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(KeePass2::FILE_VERSION_4, KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::CipherID, db->cipher().toByteArray()));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::CompressionFlags,
                                            Endian::sizedIntToBytes(static_cast<int>(db->compressionAlgo()),
                                                                    KeePass2::BYTEORDER)));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::MasterSeed, masterSeed));
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::EncryptionIV, encryptionIV));

        // Convert current Kdf to basic parameters
        QVariantMap kdfParams = KeePass2::kdfToParameters(db->kdf());

        QByteArray kdfParamBytes;
        if (!serializeVariantMap(kdfParams, kdfParamBytes)) {
            raiseError("Failed to serialise KDF parameters variant map");
            return false;
        }
        QByteArray publicCustomData = db->publicCustomData();
        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::KdfParameters, kdfParamBytes));
        if (!publicCustomData.isEmpty()) {
            CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::PublicCustomData, publicCustomData));
        }

        CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::EndOfHeader, endOfHeader));
        header.close();
        m_device = device;
        headerData = header.data();
    }
    CHECK_RETURN_FALSE(writeData(headerData));
    QByteArray headerHash = CryptoHash::hash(headerData, CryptoHash::Sha256);

    QByteArray hmacKey = KeePass2::hmacKey(masterSeed, db->transformedMasterKey());
    QByteArray headerHmac = CryptoHash::hmac(headerData, HmacBlockStream::getHmacKey(UINT64_MAX, hmacKey),
                                             CryptoHash::Sha256);
    CHECK_RETURN_FALSE(writeData(headerHash));
    CHECK_RETURN_FALSE(writeData(headerHmac));

    QScopedPointer<HmacBlockStream> hmacBlockStream;
    QScopedPointer<SymmetricCipherStream> cipherStream;

    hmacBlockStream.reset(new HmacBlockStream(device, hmacKey));
    if (!hmacBlockStream->open(QIODevice::WriteOnly)) {
        raiseError(hmacBlockStream->errorString());
        return false;
    }

    cipherStream.reset(new SymmetricCipherStream(hmacBlockStream.data(), algo,
                                                SymmetricCipher::algorithmMode(algo),
                                                SymmetricCipher::Encrypt));

    if (!cipherStream->init(finalKey, encryptionIV)) {
        raiseError(cipherStream->errorString());
        return false;
    }
    if (!cipherStream->open(QIODevice::WriteOnly)) {
        raiseError(cipherStream->errorString());
        return false;
    }

    QScopedPointer<QtIOCompressor> ioCompressor;
    if (db->compressionAlgo() == Database::CompressionNone) {
        m_device = cipherStream.data();
    } else {
        ioCompressor.reset(new QtIOCompressor(cipherStream.data()));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::WriteOnly)) {
            raiseError(ioCompressor->errorString());
            return false;
        }
        m_device = ioCompressor.data();
    }

    QHash<QByteArray, int> idMap;

    CHECK_RETURN_FALSE(writeInnerHeaderField(KeePass2::InnerHeaderFieldID::InnerRandomStreamID,
                                             Endian::sizedIntToBytes(static_cast<int>(KeePass2::ProtectedStreamAlgo::ChaCha20),
                                                                     KeePass2::BYTEORDER)));
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

    KeePass2RandomStream randomStream(KeePass2::ProtectedStreamAlgo::ChaCha20);
    if (!randomStream.init(protectedStreamKey)) {
        raiseError(randomStream.errorString());
        return false;
    }

    Kdbx4XmlWriter xmlWriter(KeePass2::FILE_VERSION_4, idMap);
    xmlWriter.writeDatabase(m_device, db, &randomStream, headerHash);

    // Explicitly close/reset streams so they are flushed and we can detect
    // errors. QIODevice::close() resets errorString() etc.
    if (ioCompressor) {
        ioCompressor->close();
    }
    if (!cipherStream->reset()) {
        raiseError(cipherStream->errorString());
        return false;
    }
    if (!hmacBlockStream->reset()) {
        raiseError(hmacBlockStream->errorString());
        return false;
    }

    if (xmlWriter.hasError()) {
        raiseError(xmlWriter.errorString());
        return false;
    }

    return true;
}

bool Kdbx4Writer::writeData(const QByteArray& data)
{
    if (m_device->write(data) != data.size()) {
        raiseError(m_device->errorString());
        return false;
    }
    return true;
}

bool Kdbx4Writer::writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(fieldId);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(static_cast<quint32>(data.size()), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool Kdbx4Writer::writeInnerHeaderField(KeePass2::InnerHeaderFieldID fieldId, const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(fieldId);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(static_cast<quint32>(data.size()), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool Kdbx4Writer::writeBinary(const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(KeePass2::InnerHeaderFieldID::Binary);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes(static_cast<quint32>(data.size() + 1), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(QByteArray(1, '\1')));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

bool Kdbx4Writer::serializeVariantMap(const QVariantMap& p, QByteArray& o)
{
    QBuffer buf(&o);
    buf.open(QIODevice::WriteOnly);
    CHECK_RETURN_FALSE(buf.write(Endian::sizedIntToBytes(KeePass2::VARIANTMAP_VERSION, KeePass2::BYTEORDER)) == 2);

    bool ok;
    QList<QString> keys = p.keys();
    for (const auto& k : keys) {
        KeePass2::VariantMapFieldType fieldType;
        QByteArray data;
        QVariant v = p.value(k);
        switch (static_cast<QMetaType::Type>(v.type())) {
        case QMetaType::Type::Int:
            fieldType = KeePass2::VariantMapFieldType::Int32;
            data = Endian::sizedIntToBytes(v.toInt(&ok), KeePass2::BYTEORDER);
            CHECK_RETURN_FALSE(ok);
            break;
        case QMetaType::Type::UInt:
            fieldType = KeePass2::VariantMapFieldType::UInt32;
            data = Endian::sizedIntToBytes(v.toUInt(&ok), KeePass2::BYTEORDER);
            CHECK_RETURN_FALSE(ok);
            break;
        case QMetaType::Type::LongLong:
            fieldType = KeePass2::VariantMapFieldType::Int64;
            data = Endian::sizedIntToBytes(v.toLongLong(&ok), KeePass2::BYTEORDER);
            CHECK_RETURN_FALSE(ok);
            break;
        case QMetaType::Type::ULongLong:
            fieldType = KeePass2::VariantMapFieldType::UInt64;
            data = Endian::sizedIntToBytes(v.toULongLong(&ok), KeePass2::BYTEORDER);
            CHECK_RETURN_FALSE(ok);
            break;
        case QMetaType::Type::QString:
            fieldType = KeePass2::VariantMapFieldType::String;
            data = v.toString().toUtf8();
            break;
        case QMetaType::Type::Bool:
            fieldType = KeePass2::VariantMapFieldType::Bool;
            data = QByteArray(1, static_cast<char>(v.toBool() ? '\1' : '\0'));
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
        QByteArray nameLenBytes = Endian::sizedIntToBytes(nameBytes.size(), KeePass2::BYTEORDER);
        QByteArray dataLenBytes = Endian::sizedIntToBytes(data.size(), KeePass2::BYTEORDER);

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
