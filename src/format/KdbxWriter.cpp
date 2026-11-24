/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "KdbxWriter.h"

#include <QBuffer>

#include "format/KdbxXmlWriter.h"

bool KdbxWriter::hasError() const
{
    return m_error;
}

QString KdbxWriter::errorString() const
{
    return m_errorStr;
}

/**
 * Write KDBX magic header numbers to a device.
 *
 * @param device output device
 * @param sig1 KDBX signature 1
 * @param sig2 KDBX signature 2
 * @param version KDBX version
 * @return true if magic numbers were written successfully
 */
bool KdbxWriter::writeMagicNumbers(QIODevice* device, quint32 sig1, quint32 sig2, quint32 version)
{
    CHECK_RETURN_FALSE(writeData(device, Endian::sizedIntToBytes<qint32>(sig1, KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(device, Endian::sizedIntToBytes<qint32>(sig2, KeePass2::BYTEORDER)));
    CHECK_RETURN_FALSE(writeData(device, Endian::sizedIntToBytes<qint32>(version, KeePass2::BYTEORDER)));

    return true;
}

/**
 * Helper method for writing bytes to the device and raising an error
 * in case of write failure.
 *
 * @param device output device
 * @param data byte contents
 * @return true on success
 */
bool KdbxWriter::writeData(QIODevice* device, const QByteArray& data)
{
    if (device->write(data) != data.size()) {
        raiseError(device->errorString());
        return false;
    }
    return true;
}

void KdbxWriter::extractDatabase(QIODevice* device, Database* db, QStringList* hiddenCharacterPlaces)
{
    if (hiddenCharacterPlaces) {
        hiddenCharacterPlaces->clear();
    }
    KdbxXmlWriter::BinaryIdxMap idxMap;
    KdbxXmlWriter writer(db->formatVersion(), idxMap);
    writer.disableInnerStreamProtection(true);
    // include attachment content; previously only names with a dangling Ref were written for KDBX 4
    writer.setWriteBinaryPool(true);
    // keep Windows line breaks (CR LF) in notes and other text
    writer.setPreserveCarriageReturns(true);
    // keep hidden characters (control characters in a password, ...) as marked base64 instead of deleting them
    writer.setStoreHiddenCharactersAsBase64(true);
    writer.writeDatabase(device, db);
    if (hiddenCharacterPlaces) {
        *hiddenCharacterPlaces = writer.hiddenCharacterPlaces();
    }
    // A failed write (full disk, closed pipe) is only visible here; without this it was reported as a success
    if (writer.hasError()) {
        raiseError(writer.errorString());
    }
}

/**
 * Raise an error. Use in case of an unexpected write error.
 *
 * @param errorMessage error message
 */
void KdbxWriter::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}
