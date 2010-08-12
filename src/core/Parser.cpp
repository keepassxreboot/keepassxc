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

    if (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "KeePassFile") {
            parseKeePassFile();
        }
        else {
            raiseError();
        }
    }

    if (!m_tmpParent->children().isEmpty()) {
        delete m_tmpParent;
        raiseError();
    }

    return !m_xml.error();
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
            m_xml.skipCurrentElement();
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
            m_meta->setRecycleBinUuid(readUuid());
        }
        else if (m_xml.name() == "RecycleBinChanged") {
            m_meta->setRecycleBinChanged(readDateTime());
        }
        else if (m_xml.name() == "EntryTemplatesGroup") {
            m_meta->setEntryTemplatesGroup(readUuid());
        }
        else if (m_xml.name() == "EntryTemplatesGroupChanged") {
            m_meta->setEntryTemplatesGroupChanged(readDateTime());
        }
        else if (m_xml.name() == "LastSelectedGroup") {
            m_meta->setLastSelectedGroup(readUuid());
        }
        else if (m_xml.name() == "LastTopVisibleGroup") {
            m_meta->setLastTopVisibleGroup(readUuid());
        }
        else if (m_xml.name() == "CustomData") {
            parseCustomData();
        }
        else {
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseCustomData()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomData");

    // TODO
}

void Parser::parseRoot()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Root");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Group") {
            Group* rootGroup = parseGroup();
            if (rootgroup) {
                rootGroup->setParent(m_db);
            }
        }
        else if (m_xml.name() == "DeletedObjects") {
            // TODO implement
        }
        else {
            m_xml.skipCurrentElement();
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
            if (iconId != 0)
                group->setIcon(iconId);
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
            // TODO implement
        }
        else if (m_xml.name() == "EnableSearching") {
            // TODO implement
        }
        else if (m_xml.name() == "LastTopVisibleEntry") {
            Uuid uuid = readUuid();
            if (uuid.isNull())
                group->setLastTopVisibleEntry(0);
            else
                group->setLastTopVisibleEntry(getEntry(uuid));
        }
        else if (m_xml.name() == "Group") {
            Group* newGroup = parseGroup();
            if (newGroup) {
                newGroup->setParent(group);
            }
        }
        else if (m_xml.name() == "Entry") {
            Entry* newEntry = parseEntry();
            if (newEntry) {
                newEntry->setGroup(group);
            }
        }
        else {
            m_xml.skipCurrentElement();
        }
    }

    return group;
}

Entry* Parser::parseEntry()
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
               entry = getEntry(uuid);
           }
        }
        else if (m_xml.name() == "IconID") {
            int iconId = readNumber();
            if (iconId != 0)
                entry->setIcon(iconId);
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
            parseEntryHistory();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
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
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseEntryHistory()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "History");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Entry") {
            // TODO implement
        }
        else {
            m_xml.skipCurrentElement();
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
            m_xml.skipCurrentElement();
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

    if (str == "True") {
        return true;
    }
    else if (str == "False") {
        return false;
    }
    else {
        raiseError();
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
    if (colorStr.length() != 7 || colorStr[0] != '#') {
        raiseError();
        return QColor();
    }

    QColor color;
    for (int i=0; i<= 2; i++) {
        QString rgbPartStr = colorStr.mid(1 + 2*i, 2);
        bool ok;
        int rgbPart = rgbPartStr.toInt(&ok);
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
        return Uuid(readBinary());
    }
}

QByteArray Parser::readBinary()
{
    return QByteArray::fromBase64(readString().toAscii());
}

Group* Parser::getGroup(const Uuid& uuid)
{
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
