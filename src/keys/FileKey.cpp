/*
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
*  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "FileKey.h"

#include <QFile>

#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"

/**
 * Read key file from device while trying to detect its file format.
 *
 * If no legacy key file format was detected, the SHA-256 hash of the
 * key file will be used, allowing usage of arbitrary files as key files.
 * In case of a detected legacy key file format, the raw byte contents
 * will be extracted from the file.
 *
 * Supported legacy formats are:
 *  - KeePass 2 XML key file
 *  - Fixed 32 byte binary
 *  - Fixed 32 byte ASCII hex-encoded binary
 *
 * Usage of legacy formats is discouraged and support for them may be
 * removed in a future version.
 *
 * @param device input device
 * @return true if key file was loaded successfully
 */
bool FileKey::load(QIODevice* device)
{
    m_type = None;

    // we may need to read the file multiple times
    if (device->isSequential()) {
        return false;
    }

    if (device->size() == 0) {
        return false;
    }

    // try different legacy key file formats
    if (!device->reset()) {
        return false;
    }
    if (loadXml(device)) {
        m_type = KeePass2XML;
        return true;
    }

    if (!device->reset()) {
        return false;
    }
    if (loadBinary(device)) {
        m_type = FixedBinary;
        return true;
    }

    if (!device->reset()) {
        return false;
    }
    if (loadHex(device)) {
        m_type = FixedBinaryHex;
        return true;
    }

    // if no legacy format was detected, generate SHA-256 hash of key file
    if (!device->reset()) {
        return false;
    }
    if (loadHashed(device)) {
        m_type = Hashed;
        return true;
    }

    return false;
}

/**
 * Load key file from path while trying to detect its file format.
 *
 * If no legacy key file format was detected, the SHA-256 hash of the
 * key file will be used, allowing usage of arbitrary files as key files.
 * In case of a detected legacy key file format, the raw byte contents
 * will be extracted from the file.
 *
 * Supported legacy formats are:
 *  - KeePass 2 XML key file
 *  - Fixed 32 byte binary
 *  - Fixed 32 byte ASCII hex-encoded binary
 *
 * Usage of legacy formats is discouraged and support for them may be
 * removed in a future version.
 *
 * @param fileName input file name
 * @param errorMsg error message if loading failed
 * @return true if key file was loaded successfully
 */
bool FileKey::load(const QString& fileName, QString* errorMsg)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        if (errorMsg) {
            *errorMsg = file.errorString();
        }
        return false;
    }
    bool result = load(&file);

    file.close();

    if (file.error()) {
        result = false;
        if (errorMsg) {
            *errorMsg = file.errorString();
        }
    }

    return result;
}

/**
 * @return key data as bytes
 */
QByteArray FileKey::rawKey() const
{
    return m_key;
}

/**
 * @return cloned \link FileKey instance
 */
FileKey* FileKey::clone() const
{
    return new FileKey(*this);
}

/**
 * Generate a new key file from random bytes.
 *
 * @param device output device
 * @param number of random bytes to generate
 */
void FileKey::create(QIODevice* device, int size)
{
    device->write(randomGen()->randomArray(size));
}

/**
 * Create a new key file from random bytes.
 *
 * @param fileName output file name
 * @param errorMsg error message if generation failed
 * @param number of random bytes to generate
 * @return true on successful creation
 */
bool FileKey::create(const QString& fileName, QString* errorMsg, int size)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        if (errorMsg) {
            *errorMsg = file.errorString();
        }
        return false;
    }
    create(&file, size);
    file.close();

    if (file.error()) {
        if (errorMsg) {
            *errorMsg = file.errorString();
        }

        return false;
    }

    return true;
}

/**
 * Load key file in legacy KeePass 2 XML format.
 *
 * @param device input device
 * @return true on success
 * @deprecated
 */
bool FileKey::loadXml(QIODevice* device)
{
    QXmlStreamReader xmlReader(device);

    if (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() != "KeyFile") {
            return false;
        }
    } else {
        return false;
    }

    bool correctMeta = false;
    QByteArray data;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Meta") {
            correctMeta = loadXmlMeta(xmlReader);
        } else if (xmlReader.name() == "Key") {
            data = loadXmlKey(xmlReader);
        }
    }

    if (!xmlReader.error() && correctMeta && !data.isEmpty()) {
        m_key = data;
        return true;
    }

    return false;
}

/**
 * Load meta data from legacy KeePass 2 XML key file.
 *
 * @param xmlReader input XML reader
 * @return true on success
 * @deprecated
 */
bool FileKey::loadXmlMeta(QXmlStreamReader& xmlReader)
{
    bool correctVersion = false;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Version") {
            if (xmlReader.readElementText() == "1.00") {
                correctVersion = true;
            }
        }
    }

    return correctVersion;
}

/**
 * Load base64 key data from legacy KeePass 2 XML key file.
 *
 * @param xmlReader input XML reader
 * @return true on success
 * @deprecated
 */
QByteArray FileKey::loadXmlKey(QXmlStreamReader& xmlReader)
{
    QByteArray data;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Data") {
            QByteArray rawData = xmlReader.readElementText().toLatin1();
            if (Tools::isBase64(rawData)) {
                data = QByteArray::fromBase64(rawData);
            }
        }
    }

    return data;
}

/**
 * Load fixed 32-bit binary key file.
 *
 * @param device input device
 * @return true on success
 * @deprecated
 */
bool FileKey::loadBinary(QIODevice* device)
{
    if (device->size() != 32) {
        return false;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(device, data) || data.size() != 32) {
        return false;
    } else {
        m_key = data;
        return true;
    }
}

/**
 * Load hex-encoded representation of fixed 32-bit binary key file.
 *
 * @param device input device
 * @return true on success
 * @deprecated
 */
bool FileKey::loadHex(QIODevice* device)
{
    if (device->size() != 64) {
        return false;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(device, data) || data.size() != 64) {
        return false;
    }

    if (!Tools::isHex(data)) {
        return false;
    }

    QByteArray key = QByteArray::fromHex(data);

    if (key.size() != 32) {
        return false;
    }

    m_key = key;
    return true;
}

/**
 * Generate SHA-256 hash of arbitrary text or binary key file.
 *
 * @param device input device
 * @return true on success
 */
bool FileKey::loadHashed(QIODevice* device)
{
    CryptoHash cryptoHash(CryptoHash::Sha256);

    QByteArray buffer;
    do {
        if (!Tools::readFromDevice(device, buffer)) {
            return false;
        }
        cryptoHash.addData(buffer);
    }
    while (!buffer.isEmpty());

    m_key = cryptoHash.result();

    return true;
}

/**
 * @return type of loaded key file
 */
FileKey::Type FileKey::type() const
{
    return m_type;
}
