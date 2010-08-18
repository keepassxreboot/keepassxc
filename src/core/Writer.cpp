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

#include "Writer.h"

#include <QtCore/QBuffer>

#include "core/Database.h"
#include "core/Metadata.h"

Writer::Writer(Database* db)
    : QObject(db)
    , m_db(db)
    , m_meta(db->metadata())
{
    m_xml.setAutoFormatting(true);
    m_xml.setAutoFormattingIndent(-1);
    m_xml.setCodec("UTF-8");

}

bool Writer::write(const QString& filename)
{
    m_xml.writeStartDocument("1.0", true);

    m_xml.writeStartElement("KeePassFile");

    writeMetadata();
    writeRoot();

    m_xml.writeEndElement();

    m_xml.writeEndDocument();

    return true; // TODO
}

void Writer::writeMetadata()
{
    m_xml.writeStartElement("Meta");

    writeString("Generator", m_meta->generator());
    writeString("DatabaseName", m_meta->name());
    writeDateTime("DatabaseNameChanged", m_meta->nameChanged());
    writeString("DatabaseDescription", m_meta->description());
    writeDateTime("DatabaseDescriptionChanged", m_meta->descriptionChanged());
    writeString("DefaultUserName", m_meta->defaultUserName());
    writeDateTime("DefaultUserNameChanged", m_meta->defaultUserNameChanged());
    writeNumber("MaintenanceHistoryDays", m_meta->maintenanceHistoryDays());
    writeMemoryProtection();
    writeCustomIcons();
    writeBool("RecycleBinEnabled", m_meta->recycleBinEnabled());
    writeUuid("RecycleBinUUID", m_meta->recycleBin());
    writeDateTime("RecycleBinChanged", m_meta->recycleBinChanged());
    writeUuid("EntryTemplatesGroup", m_meta->entryTemplatesGroup());
    writeDateTime("EntryTemplatesGroupChanged", m_meta->entryTemplatesGroupChanged());
    writeUuid("LastSelectedGroup", m_meta->lastSelectedGroup());
    writeUuid("LastTopVisibleGroup", m_meta->lastTopVisibleGroup());
    writeCustomData();

    m_xml.writeEndElement();
}

void Writer::writeMemoryProtection()
{
    m_xml.writeStartElement("MemoryProtection");

    writeBool("ProtectTitle", m_meta->protectTitle());
    writeBool("ProtectUserName", m_meta->protectUsername());
    writeBool("ProtectPassword", m_meta->protectPassword());
    writeBool("ProtectURL", m_meta->protectUrl());
    writeBool("ProtectNotes", m_meta->protectNotes());
    writeBool("AutoEnableVisualHiding", m_meta->autoEnableVisualHiding());

    m_xml.writeEndElement();
}

void Writer::writeCustomIcons()
{
    m_xml.writeStartElement("CustomIcons");

    Q_FOREACH (const Uuid& uuid, m_meta->customIcons().keys()) {
        writeIcon(uuid, m_meta->customIcons().value(uuid));
    }

    m_xml.writeEndElement();
}

void Writer::writeIcon(const Uuid& uuid, const QImage& image)
{
    m_xml.writeStartElement("Icon");

    writeUuid("UUID", uuid);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    writeBinary("Data", ba);

    m_xml.writeEndElement();
}

void Writer::writeCustomData()
{
    m_xml.writeStartElement("CustomData");

    // TODO implement

    m_xml.writeEndElement();
}

void Writer::writeRoot()
{
    Q_ASSERT(m_db->rootGroup());

    m_xml.writeStartElement("Root");

    writeGroup(m_db->rootGroup());

    m_xml.writeEndElement();
}

void Writer::writeGroup(const Group* group)
{
    m_xml.writeStartElement("Group");

    writeUuid("UUID", group->uuid());
    writeString("Name", group->name());
    writeNumber("IconID", group->iconNumber());
    writeUuid("CustomIconUUID", group->iconUuid());
    writeTimes(group->timeInfo());
    writeBool("IsExpanded", group->isExpanded());
    writeString("DefaultAutoTypeSequence", group->defaultAutoTypeSequence());
    // TODO EnableAutoType
    // TODO EnableSearching
    writeUuid("LastTopVisibleEntry", group->lastTopVisibleEntry());

    Q_FOREACH (const Group* child, group->children()) {
        writeGroup(child);
    }

    Q_FOREACH (const Entry* entry, group->entries()) {
        writeEntry(entry);
    }

    m_xml.writeEndElement();
}

void Writer::writeTimes(const TimeInfo& ti)
{
    m_xml.writeStartElement("Times");

    writeDateTime("LastModificationTime", ti.lastModificationTime());
    writeDateTime("CreationTime", ti.creationTime());
    writeDateTime("LastAccessTime", ti.lastAccessTime());
    writeDateTime("ExpiryTime", ti.expiryTime());
    writeBool("Expires", ti.expires());
    writeNumber("UsageCount", ti.usageCount());
    writeDateTime("LocationChanged", ti.locationChanged());

    m_xml.writeEndElement();
}

void Writer::writeEntry(const Entry* entry)
{
    m_xml.writeStartElement("Entry");

    writeUuid("UUID", entry->uuid());
    writeNumber("IconID", entry->iconNumber());
    writeUuid("CustomIconUUID", entry->iconUuid());
    writeColor("ForegroundColor", entry->foregroundColor());
    writeColor("BackgroundColor", entry->backgroundColor());
    writeString("OverrideURL", entry->overrideUrl());
    writeTimes(entry->timeInfo());

    Q_FOREACH (const QString& key, entry->attributes()) {
        m_xml.writeStartElement("String");
        writeString("Key", key);
        writeString("Value", entry->attributes().value(key));
        m_xml.writeEndElement();
    }

    Q_FOREACH (const QString& key, entry->attachments()) {
        m_xml.writeStartElement("Binary");
        writeString("Key", key);
        writeBinary("Value", entry->attachments().value(key));
        m_xml.writeEndElement();
    }

    writeAutoType(entry);
    writeEntryHistory(entry);

    m_xml.writeEndElement();
}

void Writer::writeAutoType(const Entry* entry)
{
    m_xml.writeStartElement("AutoType");

    writeBool("Enabled", entry->autoTypeEnabled());
    writeNumber("DataTransferObfuscation", entry->autoTypeObfuscation());
    writeString("DefaultSequence", entry->defaultAutoTypeSequence());

    Q_FOREACH (const AutoTypeAssociation& assoc, entry->autoTypeAssociations()) {
        writeAutoTypeAssoc(assoc);
    }

    m_xml.writeEndElement();
}

void Writer::writeAutoTypeAssoc(const AutoTypeAssociation& assoc)
{
    m_xml.writeStartElement("Association");

    writeString("Window", assoc.window);
    writeString("KeystrokeSequence", assoc.sequence);

    m_xml.writeEndElement();
}

void Writer::writeEntryHistory(const Entry* entry)
{
    m_xml.writeStartElement("History");

    // TODO implement
    Q_UNUSED(entry);

    m_xml.writeEndElement();
}

void Writer::writeString(const QString& qualifiedName, const QString& string)
{
    m_xml.writeTextElement(qualifiedName, string);
}

void Writer::writeNumber(const QString& qualifiedName, int number)
{
    writeString(qualifiedName, QString::number(number));
}

void Writer::writeBool(const QString& qualifiedName, bool b)
{
    if (b) {
        writeString(qualifiedName, "True");
    }
    else {
        writeString(qualifiedName, "False");
    }
}

void Writer::writeDateTime(const QString& qualifiedName, const QDateTime& dateTime)
{
    writeString(qualifiedName, dateTime.toString(Qt::ISODate));
}

void Writer::writeUuid(const QString& qualifiedName, const Uuid& uuid)
{
    writeString(qualifiedName, uuid.toBase64());
}

void Writer::writeUuid(const QString& qualifiedName, const Group* group)
{
    if (group) {
        writeUuid(qualifiedName, group->uuid());
    }
    else {
        writeUuid(qualifiedName, Uuid());
    }
}

void Writer::writeUuid(const QString& qualifiedName, const Entry* entry)
{
    if (entry) {
        writeUuid(qualifiedName, entry->uuid());
    }
    else {
        writeUuid(qualifiedName, Uuid());
    }
}

void Writer::writeBinary(const QString& qualifiedName, const QByteArray& ba)
{
    writeString(qualifiedName, QString::fromAscii(ba.toBase64()));
}

void Writer::writeColor(const QString& qualifiedName, const QColor& color)
{
    QString colorStr = QString("#%1%2%3").arg(colorPartToString(color.red()))
            .arg(colorPartToString(color.green()))
            .arg(colorPartToString(color.blue()));

    writeString(qualifiedName, colorStr);
}

QString Writer::colorPartToString(int value)
{
    QString str = QString::number(value, 16).toUpper();
    if (str.length() == 1) {
        str.prepend("0");
    }

    return str;
}
