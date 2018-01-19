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

#include "core/Group.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "format/KeePass2RandomStream.h"
#include "format/KdbxXmlReader.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

#include <QBuffer>

Database* Kdbx3Reader::readDatabaseImpl(QIODevice* device, const QByteArray& headerData,
                                        const CompositeKey& key, bool keepDatabase)
{
    Q_ASSERT(m_kdbxVersion <= KeePass2::FILE_VERSION_3_1);

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

    if (!m_db->challengeMasterSeed(m_masterSeed)) {
        raiseError(tr("Unable to issue challenge-response."));
        return nullptr;
    }

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(m_masterSeed);
    hash.addData(m_db->challengeResponseKey());
    hash.addData(m_db->transformedMasterKey());
    QByteArray finalKey = hash.result();

    SymmetricCipher::Algorithm cipher = SymmetricCipher::cipherToAlgorithm(m_db->cipher());
    SymmetricCipherStream cipherStream(device, cipher,
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

    QIODevice* xmlDevice = nullptr;
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

    QBuffer buffer;
    if (saveXml()) {
        m_xmlData = xmlDevice->readAll();
        buffer.setBuffer(&m_xmlData);
        buffer.open(QIODevice::ReadOnly);
        xmlDevice = &buffer;
    }

    Q_ASSERT(xmlDevice);

    KdbxXmlReader xmlReader(KeePass2::FILE_VERSION_3_1);
    xmlReader.readDatabase(xmlDevice, m_db.data(), &randomStream);

    if (xmlReader.hasError()) {
        raiseError(xmlReader.errorString());
        if (keepDatabase) {
            return m_db.take();
        }
        return nullptr;
    }

    Q_ASSERT(!xmlReader.headerHash().isEmpty() || m_kdbxVersion < KeePass2::FILE_VERSION_3_1);

    if (!xmlReader.headerHash().isEmpty()) {
        QByteArray headerHash = CryptoHash::hash(headerData, CryptoHash::Sha256);
        if (headerHash != xmlReader.headerHash()) {
            raiseError("Header doesn't match hash");
            return nullptr;
        }
    }

    return m_db.take();
}

bool Kdbx3Reader::readHeaderField(StoreDataStream& headerStream)
{
    QByteArray fieldIDArray = headerStream.read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("Invalid header id size");
        return false;
    }
    char fieldID = fieldIDArray.at(0);

    bool ok;
    auto fieldLen = Endian::readSizedInt<quint16>(&headerStream, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError("Invalid header field length");
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = headerStream.read(fieldLen);
        if (fieldData.size() != fieldLen) {
            raiseError("Invalid header data length");
            return false;
        }
    }

    bool headerEnd = false;
    switch (static_cast<KeePass2::HeaderFieldID>(fieldID)) {
    case KeePass2::HeaderFieldID::EndOfHeader:
        headerEnd = true;
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

    return !headerEnd;
}
