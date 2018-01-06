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

#include "Kdbx3Writer.h"

#include <QBuffer>
#include <QFile>
#include <QIODevice>

#include "core/Database.h"
#include "core/Endian.h"
#include "crypto/CryptoHash.h"
#include "crypto/kdf/AesKdf.h"
#include "crypto/Random.h"
#include "format/KeePass2RandomStream.h"
#include "format/Kdbx3XmlWriter.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

Kdbx3Writer::Kdbx3Writer()
    : m_device(0)
{
}

bool Kdbx3Writer::writeDatabase(QIODevice* device, Database* db)
{
    m_error = false;
    m_errorStr.clear();

    QByteArray masterSeed = randomGen()->randomArray(32);
    QByteArray encryptionIV = randomGen()->randomArray(16);
    QByteArray protectedStreamKey = randomGen()->randomArray(32);
    QByteArray startBytes = randomGen()->randomArray(32);
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

    QBuffer header;
    header.open(QIODevice::WriteOnly);
    m_device = &header;

    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes<qint32>(KeePass2::SIGNATURE_1, KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes<qint32>(KeePass2::SIGNATURE_2, KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes<qint32>(KeePass2::FILE_VERSION_3, KeePass2::BYTEORDER)));

    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::CipherID, db->cipher().toByteArray()));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::CompressionFlags,
                                        Endian::sizedIntToBytes<qint32>(db->compressionAlgo(),
                                                                        KeePass2::BYTEORDER)));
    auto kdf = db->kdf();
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::MasterSeed, masterSeed));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::TransformSeed, kdf->seed()));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::TransformRounds,
                                        Endian::sizedIntToBytes<qint64>(kdf->rounds(),
                                                                        KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::EncryptionIV, encryptionIV));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::ProtectedStreamKey, protectedStreamKey));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::StreamStartBytes, startBytes));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::InnerRandomStreamID,
                                        Endian::sizedIntToBytes<qint32>(static_cast<qint32>(KeePass2::ProtectedStreamAlgo::Salsa20),
                                                                        KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeHeaderField(KeePass2::HeaderFieldID::EndOfHeader, endOfHeader));

    header.close();
    m_device = device;
    QByteArray headerHash = CryptoHash::hash(header.data(), CryptoHash::Sha256);
    CHECK_RETURN_FALSE(writeData(header.data()));

    SymmetricCipher::Algorithm algo = SymmetricCipher::cipherToAlgorithm(db->cipher());
    SymmetricCipherStream cipherStream(device, algo,
                                       SymmetricCipher::algorithmMode(algo), SymmetricCipher::Encrypt);
    cipherStream.init(finalKey, encryptionIV);
    if (!cipherStream.open(QIODevice::WriteOnly)) {
        raiseError(cipherStream.errorString());
        return false;
    }
    m_device = &cipherStream;
    CHECK_RETURN_FALSE(writeData(startBytes));

    HashedBlockStream hashedStream(&cipherStream);
    if (!hashedStream.open(QIODevice::WriteOnly)) {
        raiseError(hashedStream.errorString());
        return false;
    }

    QScopedPointer<QtIOCompressor> ioCompressor;

    if (db->compressionAlgo() == Database::CompressionNone) {
        m_device = &hashedStream;
    } else {
        ioCompressor.reset(new QtIOCompressor(&hashedStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        if (!ioCompressor->open(QIODevice::WriteOnly)) {
            raiseError(ioCompressor->errorString());
            return false;
        }
        m_device = ioCompressor.data();
    }

    KeePass2RandomStream randomStream(KeePass2::ProtectedStreamAlgo::Salsa20);
    if (!randomStream.init(protectedStreamKey)) {
        raiseError(randomStream.errorString());
        return false;
    }

    Kdbx3XmlWriter xmlWriter;
    xmlWriter.writeDatabase(m_device, db, &randomStream, headerHash);

    // Explicitly close/reset streams so they are flushed and we can detect
    // errors. QIODevice::close() resets errorString() etc.
    if (ioCompressor) {
        ioCompressor->close();
    }
    if (!hashedStream.reset()) {
        raiseError(hashedStream.errorString());
        return false;
    }
    if (!cipherStream.reset()) {
        raiseError(cipherStream.errorString());
        return false;
    }

    if (xmlWriter.hasError()) {
        raiseError(xmlWriter.errorString());
    }

    return true;
}

bool Kdbx3Writer::writeData(const QByteArray& data)
{
    if (m_device->write(data) != data.size()) {
        raiseError(m_device->errorString());
        return false;
    } else {
        return true;
    }
}

bool Kdbx3Writer::writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data)
{
    Q_ASSERT(data.size() <= 65535);

    QByteArray fieldIdArr;
    fieldIdArr[0] = static_cast<char>(fieldId);
    CHECK_RETURN_FALSE(writeData(fieldIdArr));
    CHECK_RETURN_FALSE(writeData(Endian::sizedIntToBytes<qint16>(static_cast<quint16>(data.size()),
                                                                 KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(data));

    return true;
}
