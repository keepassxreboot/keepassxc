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

#include "KeePass2XmlWriter.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>

#include "core/Metadata.h"
#include "crypto/SymmetricCipher.h"

KeePass2XmlWriter::KeePass2XmlWriter()
    : m_db(0)
    , m_meta(0)
    , m_cipher(0)
{
    m_xml.setAutoFormatting(true);
    m_xml.setAutoFormattingIndent(-1); // 1 tab
    m_xml.setCodec("UTF-8");
}

void KeePass2XmlWriter::writeDatabase(QIODevice* device, Database* db, SymmetricCipher* cipher)
{
    m_db = db;
    m_meta = db->metadata();
    m_cipher = cipher;

    m_xml.setDevice(device);

    m_xml.writeStartDocument("1.0", true);

    m_xml.writeStartElement("KeePassFile");

    writeMetadata();
    writeRoot();

    m_xml.writeEndElement();

    m_xml.writeEndDocument();
}

void KeePass2XmlWriter::writeDatabase(const QString& filename, Database* db, SymmetricCipher* cipher)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    writeDatabase(&file, db, cipher);
}

void KeePass2XmlWriter::writeMetadata()
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
    writeDateTime("MasterKeyChanged", m_meta->masterKeyChanged());
    writeNumber("MasterKeyChangeRec", m_meta->masterKeyChangeRec());
    writeNumber("MasterKeyChangeForce", m_meta->masterKeyChangeForce());
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

void KeePass2XmlWriter::writeMemoryProtection()
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

void KeePass2XmlWriter::writeCustomIcons()
{
    m_xml.writeStartElement("CustomIcons");

    QHash<Uuid, QIcon> customIcons = m_meta->customIcons();
    Q_FOREACH (const Uuid& uuid, customIcons.keys()) {
        writeIcon(uuid, customIcons.value(uuid));
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeIcon(const Uuid& uuid, const QIcon& icon)
{
    m_xml.writeStartElement("Icon");

    writeUuid("UUID", uuid);

    QPixmap pixmap = icon.pixmap(16, 16);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    buffer.close();
    writeBinary("Data", ba);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeCustomData()
{
    m_xml.writeStartElement("CustomData");

    QHash<QString, QString> customFields = m_meta->customFields();
    Q_FOREACH (const QString& key, customFields.keys()) {
        writeCustomDataItem(key, customFields.value(key));
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeCustomDataItem(const QString& key, const QString& value)
{
    m_xml.writeStartElement("Item");

    writeString("Key", key);
    writeString("Value", value);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeRoot()
{
    Q_ASSERT(m_db->rootGroup());

    m_xml.writeStartElement("Root");

    writeGroup(m_db->rootGroup());
    writeDeletedObjects();

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeGroup(const Group* group)
{
    m_xml.writeStartElement("Group");

    writeUuid("UUID", group->uuid());
    writeString("Name", group->name());
    writeNumber("IconID", group->iconNumber());

    if (!group->iconUuid().isNull()) {
        writeUuid("CustomIconUUID", group->iconUuid());
    }
    writeTimes(group->timeInfo());
    writeBool("IsExpanded", group->isExpanded());
    writeString("DefaultAutoTypeSequence", group->defaultAutoTypeSequence());

    int autoTypeEnabled = group->autoTypeEnabled();
    QString autoTypeEnabledStr;
    if (autoTypeEnabled == -1) {
        autoTypeEnabledStr = "null";
    }
    else if (autoTypeEnabled == 0) {
        autoTypeEnabledStr = "false";
    }
    else {
        autoTypeEnabledStr = "true";
    }
    writeString("EnableAutoType", autoTypeEnabledStr);

    int searchingEnabed = group->searchingEnabed();
    QString searchingEnabedStr;
    if (searchingEnabed == -1) {
        searchingEnabedStr = "null";
    }
    else if (searchingEnabed == 0) {
        searchingEnabedStr = "false";
    }
    else {
        searchingEnabedStr = "true";
    }
    writeString("EnableSearching", searchingEnabedStr);

    writeUuid("LastTopVisibleEntry", group->lastTopVisibleEntry());

    Q_FOREACH (const Entry* entry, group->entries()) {
        writeEntry(entry);
    }

    Q_FOREACH (const Group* child, group->children()) {
        writeGroup(child);
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeTimes(const TimeInfo& ti)
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

void KeePass2XmlWriter::writeDeletedObjects()
{
    m_xml.writeStartElement("DeletedObjects");

    Q_FOREACH (const DeletedObject& delObj, m_db->deletedObjects()) {
        writeDeletedObject(delObj);
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeDeletedObject(const DeletedObject& delObj)
{
    m_xml.writeStartElement("DeletedObject");

    writeUuid("UUID", delObj.uuid);
    writeDateTime("DeletionTime", delObj.deletionTime);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeEntry(const Entry* entry)
{
    m_xml.writeStartElement("Entry");

    writeUuid("UUID", entry->uuid());
    writeNumber("IconID", entry->iconNumber());
    if (!entry->iconUuid().isNull()) {
        writeUuid("CustomIconUUID", entry->iconUuid());
    }
    writeColor("ForegroundColor", entry->foregroundColor());
    writeColor("BackgroundColor", entry->backgroundColor());
    writeString("OverrideURL", entry->overrideUrl());
    writeString("Tags", entry->tags());
    writeTimes(entry->timeInfo());

    Q_FOREACH (const QString& key, entry->attributes().keys()) {
        m_xml.writeStartElement("String");

        bool protect = ( ((key == "Title") && m_meta->protectTitle()) ||
                         ((key == "UserName") && m_meta->protectUsername()) ||
                         ((key == "Password") && m_meta->protectPassword()) ||
                         ((key == "URL") && m_meta->protectUrl()) ||
                         ((key == "Notes") && m_meta->protectNotes()) ||
                         entry->isAttributeProtected(key) ) &&
                       m_cipher;

        m_xml.writeStartElement("Key");
        if (protect) {
            m_xml.writeAttribute("Protected", "True");
        }
        m_xml.writeCharacters(key);
        m_xml.writeEndElement();

        if (protect) {
            writeBinary("Value", m_cipher->process(entry->attributes().value(key).toUtf8()));
        }
        else {
            writeString("Value", entry->attributes().value(key));
        }

        m_xml.writeEndElement();
    }

    Q_FOREACH (const QString& key, entry->attachments().keys()) {
        m_xml.writeStartElement("Binary");

        bool protect = entry->isAttachmentProtected(key) && m_cipher;
        m_xml.writeStartElement("Key");
        if (protect) {
            m_xml.writeAttribute("Protected", "True");
        }
        m_xml.writeCharacters(key);
        m_xml.writeEndElement();

        if (protect) {
            writeBinary("Value", m_cipher->process(entry->attachments().value(key)));
        }
        else {
            writeBinary("Value", entry->attachments().value(key));
        }

        m_xml.writeEndElement();
    }

    writeAutoType(entry);
    writeEntryHistory(entry);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeAutoType(const Entry* entry)
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

void KeePass2XmlWriter::writeAutoTypeAssoc(const AutoTypeAssociation& assoc)
{
    m_xml.writeStartElement("Association");

    writeString("Window", assoc.window);
    writeString("KeystrokeSequence", assoc.sequence);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeEntryHistory(const Entry* entry)
{
    m_xml.writeStartElement("History");

    const QList<Entry*>& historyItems = entry->historyItems();
    Q_FOREACH (const Entry* item, historyItems) {
        writeEntry(item);
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeString(const QString& qualifiedName, const QString& string)
{
    m_xml.writeTextElement(qualifiedName, string);
}

void KeePass2XmlWriter::writeNumber(const QString& qualifiedName, int number)
{
    writeString(qualifiedName, QString::number(number));
}

void KeePass2XmlWriter::writeBool(const QString& qualifiedName, bool b)
{
    if (b) {
        writeString(qualifiedName, "True");
    }
    else {
        writeString(qualifiedName, "False");
    }
}

void KeePass2XmlWriter::writeDateTime(const QString& qualifiedName, const QDateTime& dateTime)
{
    Q_ASSERT(dateTime.isValid());

    writeString(qualifiedName, dateTime.toUTC().toString(Qt::ISODate).append('Z'));
}

void KeePass2XmlWriter::writeUuid(const QString& qualifiedName, const Uuid& uuid)
{
    writeString(qualifiedName, uuid.toBase64());
}

void KeePass2XmlWriter::writeUuid(const QString& qualifiedName, const Group* group)
{
    if (group) {
        writeUuid(qualifiedName, group->uuid());
    }
    else {
        writeUuid(qualifiedName, Uuid());
    }
}

void KeePass2XmlWriter::writeUuid(const QString& qualifiedName, const Entry* entry)
{
    if (entry) {
        writeUuid(qualifiedName, entry->uuid());
    }
    else {
        writeUuid(qualifiedName, Uuid());
    }
}

void KeePass2XmlWriter::writeBinary(const QString& qualifiedName, const QByteArray& ba)
{
    writeString(qualifiedName, QString::fromAscii(ba.toBase64()));
}

void KeePass2XmlWriter::writeColor(const QString& qualifiedName, const QColor& color)
{
    QString colorStr;

    if (color.isValid()) {
      colorStr = QString("#%1%2%3").arg(colorPartToString(color.red()))
              .arg(colorPartToString(color.green()))
              .arg(colorPartToString(color.blue()));
    }

    writeString(qualifiedName, colorStr);
}

QString KeePass2XmlWriter::colorPartToString(int value)
{
    QString str = QString::number(value, 16).toUpper();
    if (str.length() == 1) {
        str.prepend("0");
    }

    return str;
}
