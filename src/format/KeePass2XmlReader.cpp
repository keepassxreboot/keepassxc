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

#include "KeePass2XmlReader.h"

#include <QtCore/QFile>

#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "format/KeePass2RandomStream.h"

KeePass2XmlReader::KeePass2XmlReader()
    : m_randomStream(0)
    , m_db(0)
    , m_meta(0)
{
}

void KeePass2XmlReader::readDatabase(QIODevice* device, Database* db, KeePass2RandomStream* randomStream)
{
    m_xml.clear();
    m_xml.setDevice(device);

    m_db = db;
    m_meta = m_db->metadata();
    m_randomStream = randomStream;

    m_tmpParent = new Group();

    if (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "KeePassFile") {
            parseKeePassFile();
        }
    }

    if (!m_xml.error()) {
        if (!m_tmpParent->children().isEmpty()) {
            qWarning("KeePass2XmlReader::readDatabase: found %d invalid group reference(s)",
                     m_tmpParent->children().size());
        }

        if (!m_tmpParent->entries().isEmpty()) {
            qWarning("KeePass2XmlReader::readDatabase: found %d invalid entry reference(s)",
                     m_tmpParent->children().size());
        }
    }

    delete m_tmpParent;
}

Database* KeePass2XmlReader::readDatabase(QIODevice* device)
{
    Database* db = new Database();
    readDatabase(device, db);
    return db;
}

Database* KeePass2XmlReader::readDatabase(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    return readDatabase(&file);
}

bool KeePass2XmlReader::hasError()
{
    return m_xml.hasError();
}

QString KeePass2XmlReader::errorString()
{
    return QString("%1\nLine %2, column %3")
            .arg(m_xml.errorString())
            .arg(m_xml.lineNumber())
            .arg(m_xml.columnNumber());
}

void KeePass2XmlReader::parseKeePassFile()
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

void KeePass2XmlReader::parseMeta()
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
        else if (m_xml.name() == "MasterKeyChanged") {
            m_meta->setMasterKeyChanged(readDateTime());
        }
        else if (m_xml.name() == "MasterKeyChangeRec") {
            m_meta->setMasterKeyChangeRec(readNumber());
        }
        else if (m_xml.name() == "MasterKeyChangeForce") {
            m_meta->setMasterKeyChangeForce(readNumber());
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

void KeePass2XmlReader::parseMemoryProtection()
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

void KeePass2XmlReader::parseCustomIcons()
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

void KeePass2XmlReader::parseIcon()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Icon");

    Uuid uuid;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            uuid = readUuid();
        }
        else if (m_xml.name() == "Data") {
            QImage icon;
            icon.loadFromData(readBinary());
            m_meta->addCustomIcon(uuid, icon);
        }
        else {
            skipCurrentElement();
        }
    }
}

void KeePass2XmlReader::parseCustomData()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomData");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Item") {
            parseCustomDataItem();
        }
        else {
            skipCurrentElement();
        }
    }
}

void KeePass2XmlReader::parseCustomDataItem()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Item");

    QString key;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {
            key = readString();
        }
        else if (m_xml.name() == "Value") {
            m_meta->addCustomField(key, readString());
        }
        else {
            skipCurrentElement();
        }
    }
}

void KeePass2XmlReader::parseRoot()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Root");

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Group") {
            Group* rootGroup = parseGroup();
            if (rootGroup) {
                Group* oldRoot = m_db->rootGroup();
                m_db->setRootGroup(rootGroup);
                delete oldRoot;
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

Group* KeePass2XmlReader::parseGroup()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Group");

    Group* group = 0;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError(1);
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
                raiseError(2);
            }
            else {
                if (iconId >= databaseIcons()->iconCount()) {
                    qWarning("KeePass2XmlReader::parseGroup: icon id \"%d\" not supported", iconId);
                }
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
                group->setAutoTypeEnabled(Group::Inherit);
            }
            else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(Group::Enable);
            }
            else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(Group::Disable);
            }
            else {
                raiseError(3);
            }

        }
        else if (m_xml.name() == "EnableSearching") {
            QString str = readString();

            if (str.compare("null", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Inherit);
            }
            else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Enable);
            }
            else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Disable);
            }
            else {
                raiseError(4);
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

void KeePass2XmlReader::parseDeletedObjects()
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

void KeePass2XmlReader::parseDeletedObject()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "DeletedObject");

    DeletedObject delObj;

    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError(5);
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

Entry* KeePass2XmlReader::parseEntry(bool history)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Entry");

    Entry* entry = 0;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {
            Uuid uuid = readUuid();
            if (uuid.isNull()) {
                raiseError(6);
            }
            else {
                if (history) {
                    entry = new Entry();
                    entry->setUuid(uuid);
                }
                else {
                    entry = getEntry(uuid);
                }
            }
        }
        else if (m_xml.name() == "IconID") {
            int iconId = readNumber();
            if (iconId < 0) {
                raiseError(7);
            }
            else {
                entry->setIcon(iconId);
            }
        }
        else if (m_xml.name() == "CustomIconUUID") {
            Uuid uuid = readUuid();
            if (!uuid.isNull()) {
                entry->setIcon(uuid);
            }
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
        else if (m_xml.name() == "Tags") {
            entry->setTags(readString());
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
                raiseError(8);
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

void KeePass2XmlReader::parseEntryString(Entry *entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "String");

    QString key;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {
            key = readString();
        }
        else if (m_xml.name() == "Value") {
            QXmlStreamAttributes attr = m_xml.attributes();
            QString value = readString();

            bool isProtected = attr.hasAttribute("Protected") && (attr.value("Protected") == "True");

            if (isProtected && !value.isEmpty()) {
                if (m_randomStream) {
                    value = m_randomStream->process(QByteArray::fromBase64(value.toAscii()));
                }
                else {
                    raiseError(9);
                }
            }

            entry->setAttribute(key, value, isProtected);
        }
        else {
            skipCurrentElement();
        }
    }
}

void KeePass2XmlReader::parseEntryBinary(Entry *entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Binary");

    QString key;
    while (!m_xml.error() && m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {
            key = readString();
        }
        else if (m_xml.name() == "Value") {
            QByteArray value = readBinary();
            QXmlStreamAttributes attr = m_xml.attributes();

            bool isProtected = attr.hasAttribute("Protected") && (attr.value("Protected") == "True");

            if (isProtected && !value.isEmpty()) {
                m_randomStream->processInPlace(value);
            }

            entry->setAttachment(key, value, isProtected);
        }
        else {
            skipCurrentElement();
        }
    }
}

void KeePass2XmlReader::parseAutoType(Entry* entry)
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

void KeePass2XmlReader::parseAutoTypeAssoc(Entry *entry)
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

void KeePass2XmlReader::parseEntryHistory(Entry* entry)
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

TimeInfo KeePass2XmlReader::parseTimes()
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

QString KeePass2XmlReader::readString()
{
    return m_xml.readElementText();
}

bool KeePass2XmlReader::readBool()
{
    QString str = readString();

    if (str.compare("True", Qt::CaseInsensitive) == 0) {
        return true;
    }
    else if (str.compare("False", Qt::CaseInsensitive) == 0) {
        return false;
    }
    else {
        raiseError(10);
        return false;
    }
}

QDateTime KeePass2XmlReader::readDateTime()
{
    QString str = readString();
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);

    if (!dt.isValid()) {
        raiseError(11);
    }

    return dt;
}

QColor KeePass2XmlReader::readColor()
{
    QString colorStr = readString();

    if (colorStr.isEmpty()) {
        return QColor();
    }

    if (colorStr.length() != 7 || colorStr[0] != '#') {
        raiseError(12);
        return QColor();
    }

    QColor color;
    for (int i=0; i<= 2; i++) {
        QString rgbPartStr = colorStr.mid(1 + 2*i, 2);
        bool ok;
        int rgbPart = rgbPartStr.toInt(&ok, 16);
        if (!ok || rgbPart > 255) {
            raiseError(13);
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

int KeePass2XmlReader::readNumber()
{
    bool ok;
    int result = readString().toInt(&ok);
    if (!ok) {
        raiseError(14);
    }
    return result;
}

Uuid KeePass2XmlReader::readUuid()
{
    QByteArray uuidBin = readBinary();
    if (uuidBin.length() != Uuid::LENGTH) {
        raiseError(15);
        return Uuid();
    }
    else {
        return Uuid(uuidBin);
    }
}

QByteArray KeePass2XmlReader::readBinary()
{
    return QByteArray::fromBase64(readString().toAscii());
}

Group* KeePass2XmlReader::getGroup(const Uuid& uuid)
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

Entry* KeePass2XmlReader::getEntry(const Uuid& uuid)
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

void KeePass2XmlReader::raiseError(int internalNumber)
{
    m_xml.raiseError(tr("Invalid database file (error no %1).").arg(QString::number(internalNumber)));
}

void KeePass2XmlReader::skipCurrentElement()
{
    qWarning("KeePass2XmlReader::skipCurrentElement: skip element \"%s\"", qPrintable(m_xml.name().toString()));
    m_xml.skipCurrentElement();
}
