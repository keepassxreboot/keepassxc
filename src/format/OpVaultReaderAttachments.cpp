/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "OpData01.h"
#include "OpVaultReader.h"

#include "core/Entry.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

/*!
 * This will \c qCritical() if unable to open the file for reading.
 * @param file the \c .attachment file to decode
 * @return \c nullptr if unable to take action, else a pair of metadata and the actual attachment bits
 * \sa https://support.1password.com/opvault-design/#attachments
 */
bool OpVaultReader::readAttachment(const QString& filePath,
                                   const QByteArray& itemKey,
                                   const QByteArray& itemHmacKey,
                                   QJsonObject& metadata,
                                   QByteArray& payload)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << QString("Unable to open \"%s\" for reading").arg(file.fileName());
        return false;
    }

    QString magic("OPCLDAT");
    QByteArray magicBytes = file.read(7);
    if (magicBytes != magic.toUtf8()) {
        qCritical() << "Expected OPCLDAT but found <<" << magicBytes.toHex() << ">>";
        return false;
    }

    QByteArray version = file.read(1);
    if (version[0] != '\001' && version[0] != '\002') {
        qCritical() << "Unexpected version number; wanted 1 or 2, got <<" << version << ">>";
        return false;
    }
    const QByteArray& metadataLenBytes = file.read(2);
    if (metadataLenBytes.size() != 2) {
        qCritical() << "Unable to read all metadata length bytes; wanted 2 bytes, got " << metadataLenBytes.size()
                    << ": <<" << metadataLenBytes.toHex() << ">>";
        return false;
    }
    const auto b0 = static_cast<unsigned char>(metadataLenBytes[0]);
    const auto b1 = static_cast<unsigned char>(metadataLenBytes[1]);
    int metadataLen = ((0xFF & b1) << 8) | (0xFF & b0);

    // no really: it's labeled "Junk" in the spec
    int junkBytesRead = file.read(2).size();
    if (junkBytesRead != 2) {
        qCritical() << "Unable to read all \"junk\" bytes; wanted 2 bytes, got " << junkBytesRead;
        return false;
    }

    const QByteArray& iconLenBytes = file.read(4);
    if (iconLenBytes.size() != 4) {
        qCritical() << "Unable to read all \"iconLen\" bytes; wanted 4 bytes, got " << iconLenBytes.size();
        return false;
    }

    int iconLen = 0;
    for (int i = 0, len = iconLenBytes.size(); i < len; ++i) {
        char ch = iconLenBytes[i];
        auto b = static_cast<unsigned char>(ch & 0xFF);
        iconLen = (b << (i * 8)) | iconLen;
    }

    QByteArray metadataJsonBytes = file.read(metadataLen);
    if (metadataJsonBytes.size() != metadataLen) {
        qCritical() << "Unable to read all bytes of metadata JSON; wanted " << metadataLen << "but read "
                    << metadataJsonBytes.size();
        return false;
    }
    QByteArray iconBytes = file.read(iconLen);
    if (iconBytes.size() != iconLen) {
        qCritical() << "Unable to read all icon bytes; wanted " << iconLen << "but read " << iconBytes.size();
        // apologies for the icon being fatal, but it would take some gear-turning
        // to re-sync where in the attach header we are
        return false;
    }

    // we don't actually _care_ what the icon bytes are,
    // but they damn well better be valid opdata01 and pass its HMAC
    OpData01 icon01;
    if (!icon01.decode(iconBytes, itemKey, itemHmacKey)) {
        qCritical() << "Unable to decipher attachment icon in " << filePath << ": " << icon01.errorString();
        return false;
    }

    QJsonParseError jsError;
    QJsonDocument jDoc = QJsonDocument::fromJson(metadataJsonBytes, &jsError);
    if (jsError.error != QJsonParseError::ParseError::NoError) {
        qCritical() << "Found invalid attachment metadata JSON at offset " << jsError.offset << ": error("
                    << jsError.error << "): " << jsError.errorString() << "\n<<" << metadataJsonBytes << ">>";
        return false;
    }
    if (!jDoc.isObject()) {
        qCritical() << "Expected " << metadataJsonBytes << "to be a JSON Object";
        return false;
    }

    metadata = jDoc.object();
    if (metadata.contains("trashed") && metadata["trashed"].toBool()) {
        return false;
    }

    OpData01 att01;
    const QByteArray encData = file.readAll();
    if (!att01.decode(encData, itemKey, itemHmacKey)) {
        qCritical() << "Unable to decipher attachment payload: " << att01.errorString();
        return false;
    }

    payload = att01.getClearText();
    return true;
}

/*!
 * \sa https://support.1password.com/opvault-design/#attachments
 */
void OpVaultReader::fillAttachments(Entry* entry,
                                    const QDir& attachmentDir,
                                    const QByteArray& entryKey,
                                    const QByteArray& entryHmacKey)
{
    /*!
     * Attachment files are named with the UUID of the item that they are attached to followed by an underscore
     * and then followed by the UUID of the attachment itself. The file is then given the extension .attachment.
     */
    auto fileFilter = QString("%1_*.attachment").arg(entry->uuidToHex().toUpper());
    const auto& attachInfoList = attachmentDir.entryInfoList(QStringList() << fileFilter, QDir::Files);
    int attachmentCount = attachInfoList.size();
    if (attachmentCount == 0) {
        return;
    }

    for (const auto& info : attachInfoList) {
        if (!info.isReadable()) {
            qCritical() << QString("Attachment file \"%1\" is not readable").arg(info.absoluteFilePath());
            continue;
        }
        fillAttachment(entry, info, entryKey, entryHmacKey);
    }
}

void OpVaultReader::fillAttachment(Entry* entry,
                                   const QFileInfo& info,
                                   const QByteArray& entryKey,
                                   const QByteArray& entryHmacKey)
{
    QJsonObject attachMetadata;
    QByteArray attachPayload;
    if (!readAttachment(info.absoluteFilePath(), entryKey, entryHmacKey, attachMetadata, attachPayload)) {
        return;
    }

    if (!attachMetadata.contains("overview")) {
        qWarning() << "Expected \"overview\" in attachment metadata";
        return;
    }

    const QString& overB64 = attachMetadata["overview"].toString();
    OpData01 over01;

    if (over01.decodeBase64(overB64, m_overviewKey, m_overviewHmacKey)) {
        QByteArray overviewJson = over01.getClearText();
        QJsonDocument overDoc = QJsonDocument::fromJson(overviewJson);
        if (overDoc.isObject()) {
            QJsonObject overObj = overDoc.object();
            attachMetadata.remove("overview");
            for (QString& key : overObj.keys()) {
                const QJsonValueRef& value = overObj[key];
                QString insertAs = key;
                for (int aa = 0; attachMetadata.contains(insertAs) && aa < 5; ++aa) {
                    insertAs = QString("%1_%2").arg(key, aa);
                }
                attachMetadata[insertAs] = value;
            }
        } else {
            qWarning() << "Expected JSON Object in \"overview\" but nope: " << overDoc;
        }
    } else {
        qCritical()
            << QString("Unable to decode attach.overview for \"%1\": %2").arg(info.fileName(), over01.errorString());
    }

    QByteArray payload;
    payload.append(QString("attachment file is actually %1 bytes\n").arg(info.size()).toUtf8());
    for (QString& key : attachMetadata.keys()) {
        const QJsonValueRef& value = attachMetadata[key];
        QByteArray valueBytes;
        if (value.isString()) {
            valueBytes = value.toString().toUtf8();
        } else if (value.isDouble()) {
            valueBytes = QString("%1").arg(value.toInt()).toUtf8();
        } else if (value.isBool()) {
            valueBytes = value.toBool() ? "true" : "false";
        } else {
            valueBytes = QString("Unexpected metadata type in attachment: %1").arg(value.type()).toUtf8();
        }
        payload.append(key.toUtf8()).append(":=").append(valueBytes).append("\n");
    }

    QString attachKey = info.baseName();
    if (attachMetadata.contains("filename")) {
        QJsonValueRef attFilename = attachMetadata["filename"];
        if (attFilename.isString()) {
            attachKey = attFilename.toString();
        } else {
            qWarning() << QString("Unexpected type of attachment \"filename\": %1").arg(attFilename.type());
        }
    }
    if (entry->attachments()->hasKey(attachKey)) {
        // Prepend a random string to the attachment name to avoid collisions
        attachKey.prepend(QString("%1_").arg(QUuid::createUuid().toString().mid(1, 5)));
    }

    entry->attachments()->set(attachKey, attachPayload);
}
