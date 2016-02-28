/*
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
#include <QXmlStreamReader>

#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"

FileKey::FileKey()
{
}

bool FileKey::load(QIODevice* device)
{
    // we may need to read the file multiple times
    if (device->isSequential()) {
        return false;
    }

    if (device->size() == 0) {
        return false;
    }

    // try different key file formats

    if (!device->reset()) {
        return false;
    }
    if (loadXml(device)) {
        return true;
    }

    if (!device->reset()) {
        return false;
    }
    if (loadBinary(device)) {
        return true;
    }

    if (!device->reset()) {
        return false;
    }
    if (loadHex(device)) {
        return true;
    }

    if (!device->reset()) {
        return false;
    }
    if (loadHashed(device)) {
        return true;
    }

    return false;
}

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

QByteArray FileKey::rawKey() const
{
    return m_key;
}

FileKey* FileKey::clone() const
{
    return new FileKey(*this);
}

void FileKey::create(QIODevice* device)
{
    QXmlStreamWriter xmlWriter(device);

    xmlWriter.writeStartDocument("1.0");

    xmlWriter.writeStartElement("KeyFile");

    xmlWriter.writeStartElement("Meta");

    xmlWriter.writeTextElement("Version", "1.00");

    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Key");

    QByteArray data = randomGen()->randomArray(32);
    xmlWriter.writeTextElement("Data", QString::fromLatin1(data.toBase64()));

    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
}

bool FileKey::create(const QString& fileName, QString* errorMsg)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        if (errorMsg) {
            *errorMsg = file.errorString();
        }
        return false;
    }
    create(&file);

    file.close();

    if (file.error()) {
        if (errorMsg) {
            *errorMsg = file.errorString();
        }

        return false;
    }
    else {
        return true;
    }
}

bool FileKey::loadXml(QIODevice* device)
{
    QXmlStreamReader xmlReader(device);

    if (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() != "KeyFile") {
            return false;
        }
    }
    else {
        return false;
    }

    bool correctMeta = false;
    QByteArray data;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Meta") {
            correctMeta = loadXmlMeta(xmlReader);
        }
        else if (xmlReader.name() == "Key") {
            data = loadXmlKey(xmlReader);
        }
    }

    if (!xmlReader.error() && correctMeta && !data.isEmpty()) {
        m_key = data;
        return true;
    }
    else {
        return false;
    }
}

bool FileKey::loadXmlMeta(QXmlStreamReader& xmlReader)
{
    bool corectVersion = false;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Version") {
            // TODO: error message about incompatible key file version
            if (xmlReader.readElementText() == "1.00") {
                corectVersion = true;
            }
        }
    }

    return corectVersion;
}

QByteArray FileKey::loadXmlKey(QXmlStreamReader& xmlReader)
{
    QByteArray data;

    while (!xmlReader.error() && xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "Data") {
            // TODO: do we need to enforce a specific data.size()?
            QByteArray rawData = xmlReader.readElementText().toLatin1();
            if (Tools::isBase64(rawData)) {
                data = QByteArray::fromBase64(rawData);
            }
        }
    }

    return data;
}

bool FileKey::loadBinary(QIODevice* device)
{
    if (device->size() != 32) {
        return false;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(device, data) || data.size() != 32) {
        return false;
    }
    else {
        m_key = data;
        return true;
    }
}

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

bool FileKey::loadHashed(QIODevice* device)
{
    CryptoHash cryptoHash(CryptoHash::Sha256);

    QByteArray buffer;
    do {
        if (!Tools::readFromDevice(device, buffer)) {
            return false;
        }
        cryptoHash.addData(buffer);
    } while (!buffer.isEmpty());

    m_key = cryptoHash.result();

    return true;
}
