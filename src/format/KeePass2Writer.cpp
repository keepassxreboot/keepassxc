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

#include <QtCore/QFile>
#include <QtCore/QIODevice>

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
    m_errorStr = QString();

    m_device = device;

    QByteArray masterSeed = Random::randomArray(32);
    QByteArray encryptionIV = Random::randomArray(16);
    QByteArray protectedStreamKey = Random::randomArray(32);
    QByteArray startBytes = Random::randomArray(32);
    QByteArray endOfHeader = "\r\n\r\n";

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(masterSeed);
    Q_ASSERT(!db->transformedMasterKey().isEmpty());
    hash.addData(db->transformedMasterKey());
    QByteArray finalKey = hash.result();


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

    SymmetricCipherStream cipherStream(device, SymmetricCipher::Aes256, SymmetricCipher::Cbc,
                                       SymmetricCipher::Encrypt, finalKey, encryptionIV);
    cipherStream.open(QIODevice::WriteOnly);
    m_device = &cipherStream;
    CHECK_RETURN(writeData(startBytes));

    HashedBlockStream hashedStream(&cipherStream);
    hashedStream.open(QIODevice::WriteOnly);

    QScopedPointer<QtIOCompressor> ioCompressor;

    if (db->compressionAlgo() == Database::CompressionNone) {
        m_device = &hashedStream;
    }
    else {
        ioCompressor.reset(new QtIOCompressor(&hashedStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        ioCompressor->open(QIODevice::WriteOnly);
        m_device = ioCompressor.data();
    }

    KeePass2RandomStream randomStream(protectedStreamKey);

    KeePass2XmlWriter xmlWriter;
    xmlWriter.writeDatabase(m_device, db, &randomStream);
}

bool KeePass2Writer::writeData(const QByteArray& data)
{
    if (m_device->write(data) != data.size()) {
        m_error = true;
        m_errorStr = m_device->errorString();
        return false;
    }
    else {
        return true;
    }
}

bool KeePass2Writer::writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data)
{
    QByteArray fieldIdArr;
    fieldIdArr[0] = fieldId;
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::int16ToBytes(data.size(), KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}

void KeePass2Writer::writeDatabase(const QString& filename, Database* db)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    writeDatabase(&file, db);
}

bool KeePass2Writer::error()
{
    return m_error;
}

QString KeePass2Writer::errorString()
{
    return m_errorStr;
}
