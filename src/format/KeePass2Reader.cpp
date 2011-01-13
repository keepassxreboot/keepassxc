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

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QIODevice>

#include "core/Database.h"
#include "crypto/CryptoHash.h"
#include "format/KeePass2.h"
#include "format/KeePass2XmlReader.h"
#include "streams/HashedBlockStream.h"
#include "streams/QtIOCompressor"
#include "streams/SymmetricCipherStream.h"

Database* KeePass2Reader::readDatabase(QIODevice* device, const CompositeKey& key)
{
    m_db = new Database();
    m_device = device;
    m_error = false;
    m_errorStr = QString();
    m_headerEnd = false;

    bool ok;

    quint32 signature1 = Endian::readUInt32(m_device, KeePass2::BYTEORDER, &ok);
    if (!ok || signature1 != KeePass2::SIGNATURE_1) {
        raiseError("1");
        return 0;
    }

    quint32 signature2 = Endian::readUInt32(m_device, KeePass2::BYTEORDER, &ok);
    if (!ok || signature2 != KeePass2::SIGNATURE_2) {
        raiseError("2");
        return 0;
    }

    quint32 version = Endian::readUInt32(m_device, KeePass2::BYTEORDER, &ok) & KeePass2::FILE_VERSION_CRITICAL_MASK;
    quint32 expectedVersion = KeePass2::FILE_VERSION & KeePass2::FILE_VERSION_CRITICAL_MASK;
    // TODO do we support old Kdbx versions?
    if (!ok || (version != expectedVersion)) {
        raiseError("3");
        return 0;
    }

    while (readHeaderField() && !error()) {
    }

    // TODO check if all header fields have been parsed

    QByteArray transformedMasterKey = key.transform(m_db->transformSeed(), m_db->transformRounds());
    m_db->setTransformedMasterKey(transformedMasterKey);

    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(m_masterSeed);
    hash.addData(transformedMasterKey);
    QByteArray finalKey = hash.result();

    SymmetricCipherStream cipherStream(device, SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt, finalKey, m_encryptionIV);
    cipherStream.open(QIODevice::ReadOnly);

    QByteArray realStart = cipherStream.read(32);

    if (realStart != m_streamStartBytes) {
        raiseError("4");
        return 0;
    }

    HashedBlockStream hashedStream(&cipherStream);
    hashedStream.open(QIODevice::ReadOnly);

    QIODevice* xmlDevice;
    QScopedPointer<QtIOCompressor> ioCompressor;

    if (m_db->compressionAlgo() == Database::CompressionNone) {
        xmlDevice = &hashedStream;
    }
    else {
        ioCompressor.reset(new QtIOCompressor(&hashedStream));
        ioCompressor->setStreamFormat(QtIOCompressor::GzipFormat);
        ioCompressor->open(QIODevice::ReadOnly);
        xmlDevice = ioCompressor.data();
    }

    QByteArray protectedStreamKey = CryptoHash::hash(m_protectedStreamKey, CryptoHash::Sha256);
    QByteArray protectedStreamIv("\xE8\x30\x09\x4B\x97\x20\x5D\x2A");

    SymmetricCipher protectedStream(SymmetricCipher::Salsa20, SymmetricCipher::Stream, SymmetricCipher::Decrypt,
                                    protectedStreamKey, protectedStreamIv);

    KeePass2XmlReader xmlReader;
    xmlReader.readDatabase(xmlDevice, m_db, &protectedStream);
    // TODO forward error messages from xmlReader
    return m_db;
}

Database* KeePass2Reader::readDatabase(const QString& filename, const CompositeKey& key)
{
    QFile file(filename);
    file.open(QFile::ReadOnly);
    Database* db = readDatabase(&file, key);
    // TODO check for QFile errors
    return db;
}

bool KeePass2Reader::error()
{
    return m_error;
}

QString KeePass2Reader::errorString()
{
    // TODO
    return QString();
}

void KeePass2Reader::raiseError(const QString& str)
{
    // TODO
    qWarning("KeePass2Reader error: %s", qPrintable(str));
    m_error = true;
}

bool KeePass2Reader::readHeaderField()
{
    QByteArray fieldIDArray = m_device->read(1);
    if (fieldIDArray.size() != 1) {
        raiseError("");
        return false;
    }
    quint8 fieldID = fieldIDArray.at(0);

    bool ok;
    quint16 fieldLen = Endian::readUInt16(m_device, KeePass2::BYTEORDER, &ok);
    if (!ok) {
        raiseError("");
        return false;
    }

    QByteArray fieldData;
    if (fieldLen != 0) {
        fieldData = m_device->read(fieldLen);
        if (fieldData.size() != fieldLen) {
            raiseError("");
            return false;
        }
    }

    switch (fieldID) {
    case KeePass2::EndOfHeader:
        m_headerEnd = true;
        break;

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
        setTransformSeed(fieldData);
        break;

    case KeePass2::TransformRounds:
        setTansformRounds(fieldData);
        break;

    case KeePass2::EncryptionIV:
        setEncryptionIV(fieldData);
        break;

    case KeePass2::ProtectedStreamKey:
        setProtectedStreamKey(fieldData);
        break;

    case KeePass2::StreamStartBytes:
        setStreamStartBytes(fieldData);
        break;

    case KeePass2::InnerRandomStreamID:
        setInnerRandomStreamID(fieldData);
        break;

    default:
        qWarning("Unknown header field read: id=%d", fieldID);
        break;
    }

    return !m_headerEnd;
}

void KeePass2Reader::setCipher(const QByteArray& data)
{
    if (data.size() != Uuid::LENGTH) {
        raiseError("");
    }
    else {
        Uuid uuid(data);

        if (uuid != KeePass2::CIPHER_AES) {
            raiseError("");
        }
        else {
            m_db->setCipher(uuid);
        }
    }
}

void KeePass2Reader::setCompressionFlags(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("");
    }
    else {
        quint32 id = Endian::bytesToUInt32(data, KeePass2::BYTEORDER);

        if (id > Database::CompressionAlgorithmMax) {
            raiseError("");
        }
        else {
            m_db->setCompressionAlgo(static_cast<Database::CompressionAlgorithm>(id));
        }
    }
}

void KeePass2Reader::setMasterSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("");
    }
    else {
        m_masterSeed = data;
    }
}

void KeePass2Reader::setTransformSeed(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("");
    }
    else {
        m_db->setTransformSeed(data);
    }
}

void KeePass2Reader::setTansformRounds(const QByteArray& data)
{
    if (data.size() != 8) {
        raiseError("");
    }
    else {
        m_db->setTransformRounds(Endian::bytesToUInt64(data, KeePass2::BYTEORDER));
    }
}

void KeePass2Reader::setEncryptionIV(const QByteArray& data)
{
    if (data.size() != 16) {
        raiseError("");
    }
    else {
        m_encryptionIV = data;
    }
}

void KeePass2Reader::setProtectedStreamKey(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("");
    }
    else {
        m_protectedStreamKey = data;
    }
}

void KeePass2Reader::setStreamStartBytes(const QByteArray& data)
{
    if (data.size() != 32) {
        raiseError("");
    }
    else {
        m_streamStartBytes = data;
    }
}

void KeePass2Reader::setInnerRandomStreamID(const QByteArray& data)
{
    if (data.size() != 4) {
        raiseError("");
    }
    else {
        quint32 id = Endian::bytesToUInt32(data, KeePass2::BYTEORDER);

        if (id != KeePass2::Salsa20) {
            raiseError("");
        }
    }
}
