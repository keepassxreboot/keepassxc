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

#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "format/KeePass2RandomStream.h"
#include "format/KeePass2XmlWriter.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

#define CHECK_RETURN(x) if (!(x)) return;
#define CHECK_RETURN_FALSE(x) if (!(x)) return false;

KeePass2Writer::KeePass2Writer()
    : m_device(0)
    , m_error(false)
{
}

void KeePass2Writer::writeDatabase(QIODevice* device, Database* db)
{
    m_error = false;
    m_errorStr.clear();

    QByteArray masterSeed = randomGen()->randomArray(32);
    QByteArray encryptionIV = randomGen()->randomArray(16);
    QByteArray protectedStreamKey = randomGen()->randomArray(32);
    QByteArray startBytes = randomGen()->randomArray(32);
    QByteArray endOfHeader = "\r\n\r\n";

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(masterSeed);
    Q_ASSERT(!db->transformedMasterKey().isEmpty());
    hash.addData(db->transformedMasterKey());
    QByteArray finalKey = hash.result();

    QBuffer header;
    header.open(QIODevice::WriteOnly);
    m_device = &header;

    CHECK_RETURN(writeData(Endian::int32ToBytes(KeePass2::SIGNATURE_1, KeePass2::BYTEORDER)));
    CHECK_RETURN(writeData(Endian::int32ToBytes(KeePass2::SIGNATURE_2, KeePass2::BYTEORDER)));
    CHECK_RETURN(writeData(Endian::int32ToBytes(KeePass2::FILE_VERSION, KeePass2::BYTEORDER)));

    CHECK_RETURN(writeHeaderField(KeePass2::CipherID, db->cipher().toByteArray()));
    CHECK_RETURN(writeHeaderField(KeePass2::CompressionFlags,
                                  Endian::int32ToBytes(db->compressionAlgo(),
                                                       KeePass2::BYTEORDER)));
    CHECK_RETURN(writeHeaderField(KeePass2::MasterSeed, masterSeed));
    CHECK_RETURN(writeHeaderField(KeePass2::TransformSeed, db->transformSeed()));
    CHECK_RETURN(writeHeaderField(KeePass2::TransformRounds,
                                  Endian::int64ToBytes(db->transformRounds(),
                                                       KeePass2::BYTEORDER)));
    CHECK_RETURN(writeHeaderField(KeePass2::EncryptionIV, encryptionIV));
    CHECK_RETURN(writeHeaderField(KeePass2::ProtectedStreamKey, protectedStreamKey));
    CHECK_RETURN(writeHeaderField(KeePass2::StreamStartBytes, startBytes));
    CHECK_RETURN(writeHeaderField(KeePass2::InnerRandomStreamID,
                                  Endian::int32ToBytes(KeePass2::Salsa20,
                                                       KeePass2::BYTEORDER)));
    CHECK_RETURN(writeHeaderField(KeePass2::EndOfHeader, endOfHeader));

    header.close();
    m_device = device;
    QByteArray headerHash = CryptoHash::hash(header.data(), CryptoHash::Sha256);
    CHECK_RETURN(writeData(header.data()));

    SymmetricCipherStream cipherStream(device, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                       SymmetricCipher::Encrypt);
    cipherStream.init(finalKey, encryptionIV);
    if (!cipherStream.open(QIODevice::WriteOnly)) {
        raiseError(cipherStream.errorString());
        return;
    }
    m_device = &cipherStream;
    CHECK_RETURN(writeData(startBytes));

    HashedBlockStream hashedStream(&cipherStream);
    if (!hashedStream.open(QIODevice::WriteOnly)) {
        raiseError(hashedStream.errorString());
        return;
    }

    QScopedPointer<QtIOCompressor> ioCompressor;

    if (db->compressionAlgo() == Database::CompressionNone) {
        m_device = &hashedStream;
    }
    else {
        ioCompressor.reset(new QtIOCompressor(&hashedStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::WriteOnly)) {
            raiseError(ioCompressor->errorString());
            return;
        }
        m_device = ioCompressor.data();
    }

    KeePass2RandomStream randomStream;
    if (!randomStream.init(protectedStreamKey)) {
        raiseError(randomStream.errorString());
        return;
    }

    KeePass2XmlWriter xmlWriter;
    xmlWriter.writeDatabase(m_device, db, &randomStream, headerHash);

    // Explicitly close/reset streams so they are flushed and we can detect
    // errors. QIODevice::close() resets errorString() etc.
    if (ioCompressor) {
        ioCompressor->close();
    }
    if (!hashedStream.reset()) {
        raiseError(hashedStream.errorString());
        return;
    }
    if (!cipherStream.reset()) {
        raiseError(cipherStream.errorString());
        return;
    }

    if (xmlWriter.hasError()) {
        raiseError(xmlWriter.errorString());
    }
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

bool KeePass2Writer::writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data)
{
    Q_ASSERT(data.size() <= 65535);

    QByteArray fieldIdArr;
    fieldIdArr[0] = fieldId;
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::int16ToBytes(static_cast<quint16>(data.size()),
                                                      KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

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
