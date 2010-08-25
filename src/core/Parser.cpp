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

#include "Parser.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>

#include "Database.h"
#include "Metadata.h"

Parser::Parser(Database* db)
{
    m_db = db;
    m_meta = db->metadata();
}

bool Parser::parse(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    m_xml.setDevice(&file);

    m_tmpParent = new Group();
    m_tmpParent->setParent(m_db);

    if (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "KeePassFile") {
            parseKeePassFile();
        }
    }

    if (!m_xml.error() && !m_tmpParent->children().isEmpty()) {
        raiseError();
    }

    delete m_tmpParent;

    return !m_xml.error();
}

QString Parser::errorMsg()
{
    return QString("%1\nLine %2, column %3")
            .arg(m_xml.errorString())
            .arg(m_xml.lineNumber())
            .arg(m_xml.columnNumber());
}

void Parser::parseKeePassFile()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "KeePassFile");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Meta") {
            parseMeta();
        }
        else if (m_xml.name() == "Root") {
            parseRoot();
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseMeta()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Meta");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Generator") {
            m_meta->setGenerator(readString());
        }
        else if (m_xml.name() == "DatabaseName") {
            m_meta->setName(readString());
        }
        else if (m_xml.name() == "DatabaseNameChanged") {
            m_meta->setNameChanged(readDateTime());
        }
        else if (m_xml.name() == "DatabaseDescription") {
            m_meta->setDescription(readString());
        }
        else if (m_xml.name() == "DatabaseDescriptionChanged") {
            m_meta->setDescriptionChanged(readDateTime());
        }
        else if (m_xml.name() == "DefaultUserName") {
            m_meta->setDefaultUserName(readString());
        }
        else if (m_xml.name() == "DefaultUserNameChanged") {
            m_meta->setDefaultUserNameChanged(readDateTime());
        }
        else if (m_xml.name() == "MaintenanceHistoryDays") {
            m_meta->setMaintenanceHistoryDays(readNumber());
        }
        else if (m_xml.name() == "MemoryProtection") {
            parseMemoryProtection();
        }
        else if (m_xml.name() == "CustomIcons") {
            parseCustomIcons();
        }
        else if (m_xml.name() == "RecycleBinEnabled") {
            m_meta->setRecycleBinEnabled(readBool());
        }
        else if (m_xml.name() == "RecycleBinUUID") {
            m_meta->setRecycleBin(getGroup(readUuid()));
        }
        else if (m_xml.name() == "RecycleBinChanged") {
            m_meta->setRecycleBinChanged(readDateTime());
        }
        else if (m_xml.name() == "EntryTemplatesGroup") {
            m_meta->setEntryTemplatesGroup(getGroup(readUuid()));
        }
        else if (m_xml.name() == "EntryTemplatesGroupChanged") {
            m_meta->setEntryTemplatesGroupChanged(readDateTime());
        }
        else if (m_xml.name() == "LastSelectedGroup") {
            m_meta->setLastSelectedGroup(getGroup(readUuid()));
        }
        else if (m_xml.name() == "LastTopVisibleGroup") {
            m_meta->setLastTopVisibleGroup(getGroup(readUuid()));
        }
        else if (m_xml.name() == "CustomData") {
            parseCustomData();
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseMemoryProtection()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "MemoryProtection");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "ProtectTitle") {
            m_meta->setProtectTitle(readBool());
        }
        else if (m_xml.name() == "ProtectUserName") {
            m_meta->setProtectUsername(readBool());
        }
        else if (m_xml.name() == "ProtectPassword") {
            m_meta->setProtectPassword(readBool());
        }
        else if (m_xml.name() == "ProtectURL") {
            m_meta->setProtectUrl(readBool());
        }
        else if (m_xml.name() == "ProtectNotes") {
            m_meta->setProtectNotes(readBool());
        }
        else if (m_xml.name() == "AutoEnableVisualHiding") {
            m_meta->setAutoEnableVisualHiding(readBool());
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseCustomIcons()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomIcons");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Icon") {
            parseIcon();
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseIcon()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Icon");

    Uuid uuid;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            uuid = readUuid();
        }
        else if (m_xml.name() == "Data") {
            QImage image;
            image.loadFromData(readBinary());
            m_meta->addCustomIcon(uuid, image);
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseCustomData()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomData");

    // TODO implement
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        skipCurrentElement();
    }
}

void Parser::parseRoot()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Root");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Group") {
            Group* rootGroup = parseGroup();
            if (rootGroup) {
                rootGroup->setParent(m_db);
            }
        }
        else if (m_xml.name() == "DeletedObjects") {
            parseDeletedObjects();
        }
        else {
            skipCurrentElement();
        }
    }
}

Group* Parser::parseGroup()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Group");

    Group* group = 0;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError();
            }
            else {
                group = getGroup(uuid);
           }
        }
        else if (m_xml.name() == "Name") {
            group->setName(readString());
        }
        else if (m_xml.name() == "Notes") {
            group->setNotes(readString());
        }
        else if (m_xml.name() == "IconID") {
            int iconId = readNumber();
            if (iconId < 0) {
                raiseError();
            }
            else if (iconId != 0) {
                group->setIcon(iconId);
            }
        }
        else if (m_xml.name() == "CustomIconUUID") {
            Uuid uuid = readUuid();
            if (!uuid.isNull()) {
                group->setIcon(uuid);
            }
        }
        else if (m_xml.name() == "Times") {
            group->setTimeInfo(parseTimes());
        }
        else if (m_xml.name() == "IsExpanded") {
            group->setExpanded(readBool());
        }
        else if (m_xml.name() == "DefaultAutoTypeSequence") {
            group->setDefaultAutoTypeSequence(readString());
        }
        else if (m_xml.name() == "EnableAutoType") {
            QString str = readString();

            if (str.compare("null", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(-1);
            }
            else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(1);
            }
            else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(0);
            }
            else {
                raiseError();
            }

        }
        else if (m_xml.name() == "EnableSearching") {
            QString str = readString();

            if (str.compare("null", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(-1);
            }
            else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(1);
            }
            else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(0);
            }
            else {
                raiseError();
            }
        }
        else if (m_xml.name() == "LastTopVisibleEntry") {
            group->setLastTopVisibleEntry(getEntry(readUuid()));
        }
        else if (m_xml.name() == "Group") {
            Group* newGroup = parseGroup();
            if (newGroup) {
                newGroup->setParent(group);
            }
        }
        else if (m_xml.name() == "Entry") {
            Entry* newEntry = parseEntry(false);
            if (newEntry) {
                newEntry->setGroup(group);
            }
        }
        else {
            skipCurrentElement();
        }
    }

    return group;
}

void Parser::parseDeletedObjects()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "DeletedObjects");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "DeletedObject") {
            parseDeletedObject();
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseDeletedObject()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "DeletedObject");

    DeletedObject delObj;

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError();
            }
            else {
                delObj.uuid = uuid;
            }
        }
        else if (m_xml.name() == "DeletionTime") {
            delObj.deletionTime = readDateTime();
        }
        else {
            skipCurrentElement();
        }
    }

    m_db->addDeletedObject(delObj);
}

Entry* Parser::parseEntry(bool history)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Entry");

    Entry* entry = 0;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError();
            }
            else {
                if (history) {
                    entry = new Entry();
                }
                else {
                    entry = getEntry(uuid);
                }
            }
        }
        else if (m_xml.name() == "IconID") {
            int iconId = readNumber();
            if (iconId < 0) {
                raiseError();
            }
            else if (iconId != 0) {
                entry->setIcon(iconId);
            }
        }
        else if (m_xml.name() == "CustomIconUUID") {
            Uuid uuid = readUuid();
            if (!uuid.isNull())
                entry->setIcon(uuid);
        }
        else if (m_xml.name() == "ForegroundColor") {
            entry->setForegroundColor(readColor());
        }
        else if (m_xml.name() == "BackgroundColor") {
            entry->setBackgroundColor(readColor());
        }
        else if (m_xml.name() == "OverrideURL") {
            entry->setOverrideUrl(readString());
        }
        else if (m_xml.name() == "Times") {
            entry->setTimeInfo(parseTimes());
        }
        else if (m_xml.name() == "String") {
            parseEntryString(entry);
        }
        else if (m_xml.name() == "Binary") {
            parseEntryBinary(entry);
        }
        else if (m_xml.name() == "AutoType") {
            parseAutoType(entry);
        }
        else if (m_xml.name() == "History") {
            if (history) {
                raiseError();
            }
            else {
                parseEntryHistory(entry);
            }
        }
        else {
            skipCurrentElement();
        }
    }

    return entry;
}

void Parser::parseEntryString(Entry *entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "String");

    QString key;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {
            key = readString();
        }
        else if (m_xml.name() == "Value") {
            entry->addAttribute(key, readString());
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseEntryBinary(Entry *entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Binary");

    QString key;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {
            key = readString();
        }
        else if (m_xml.name() == "Value") {
            entry->addAttachment(key, readBinary());
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseAutoType(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "AutoType");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Enabled") {
            entry->setAutoTypeEnabled(readBool());
        }
        else if (m_xml.name() == "DataTransferObfuscation") {
            entry->setAutoTypeObfuscation(readNumber());
        }
        else if (m_xml.name() == "DefaultSequence") {
            entry->setDefaultAutoTypeSequence(readString());
        }
        else if (m_xml.name() == "Association") {
            parseAutoTypeAssoc(entry);
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseAutoTypeAssoc(Entry *entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Association");

    AutoTypeAssociation assoc;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Window") {
            assoc.window = readString();
        }
        else if (m_xml.name() == "KeystrokeSequence") {
            assoc.sequence = readString();
            entry->addAutoTypeAssociation(assoc);
        }
        else {
            skipCurrentElement();
        }
    }
}

void Parser::parseEntryHistory(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "History");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Entry") {
            Entry* historyItem = parseEntry(true);
            entry->addHistoryItem(historyItem);
        }
        else {
            skipCurrentElement();
        }
    }
}

TimeInfo Parser::parseTimes()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Times");

    TimeInfo timeInfo;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "LastModificationTime") {
            timeInfo.setLastModificationTime(readDateTime());
        }
        else if (m_xml.name() == "CreationTime") {
            timeInfo.setCreationTime(readDateTime());
        }
        else if (m_xml.name() == "LastAccessTime") {
            timeInfo.setLastAccessTime(readDateTime());
        }
        else if (m_xml.name() == "ExpiryTime") {
            timeInfo.setExpiryTime(readDateTime());
        }
        else if (m_xml.name() == "Expires") {
            timeInfo.setExpires(readBool());
        }
        else if (m_xml.name() == "UsageCount") {
            timeInfo.setUsageCount(readNumber());
        }
        else if (m_xml.name() == "LocationChanged") {
            timeInfo.setLocationChanged(readDateTime());
        }
        else {
            skipCurrentElement();
        }
    }

    return timeInfo;
}

QString Parser::readString()
{
    return m_xml.readElementText();
}

bool Parser::readBool()
{
    QString str = readString();

    if (str.compare("True", Qt::CaseInsensitive) == 0) {
        return true;
    }
    else if (str.compare("False", Qt::CaseInsensitive) == 0) {
        return false;
    }
    else {
        raiseError();
        return false;
    }
}

QDateTime Parser::readDateTime()
{
    QString str = readString();
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);

    if (!dt.isValid()) {
        raiseError();
    }

    return dt;
}

QColor Parser::readColor()
{
    QString colorStr = readString();

    if (colorStr.isEmpty()) {
        return QColor();
    }

    if (colorStr.length() != 7 || colorStr[0] != '#') {
        raiseError();
        return QColor();
    }

    QColor color;
    for (int i=0; i<= 2; i++) {
        QString rgbPartStr = colorStr.mid(1 + 2*i, 2);
        bool ok;
        int rgbPart = rgbPartStr.toInt(&ok, 16);
        if (!ok || rgbPart > 255) {
            raiseError();
            return QColor();
        }

        if (i == 0) {
            color.setRed(rgbPart);
        }
        else if (i == 1) {
            color.setGreen(rgbPart);
        }
        else {
            color.setBlue(rgbPart);
        }
    }

    return color;
}

int Parser::readNumber()
{
    bool ok;
    int result = readString().toInt(&ok);
    if (!ok) {
        raiseError();
    }
    return result;
}

Uuid Parser::readUuid()
{
    QByteArray uuidBin = readBinary();
    if (uuidBin.length() != Uuid::length) {
        raiseError();
        return Uuid();
    }
    else {
        return Uuid(uuidBin);
    }
}

QByteArray Parser::readBinary()
{
    return QByteArray::fromBase64(readString().toAscii());
}

Group* Parser::getGroup(const Uuid& uuid)
{
    if (uuid.isNull()) {
        return 0;
    }

    Q_FOREACH (Group* group, m_groups) {
        if (group->uuid() == uuid) {
            return group;
        }
    }

    Group* group = new Group();
    group->setUuid(uuid);
    group->setParent(m_tmpParent);
    m_groups << group;
    return group;
}

Entry* Parser::getEntry(const Uuid& uuid)
{
    if (uuid.isNull()) {
        return 0;
    }

    Q_FOREACH (Entry* entry, m_entries) {
        if (entry->uuid() == uuid) {
            return entry;
        }
    }

    Entry* entry = new Entry();
    entry->setUuid(uuid);
    entry->setGroup(m_tmpParent);
    m_entries << entry;
    return entry;
}

void Parser::raiseError()
{
    m_xml.raiseError(tr("Invalid database file"));
}

void Parser::skipCurrentElement()
{
    qDebug() << "Parser::skipCurrentElement(): skip: " << m_xml.name();
    m_xml.skipCurrentElement();
}
