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

#include <QBuffer>
#include <QFile>

#include "core/Metadata.h"
#include "format/KeePass2RandomStream.h"
#include "streams/QtIOCompressor"

KeePass2XmlWriter::KeePass2XmlWriter()
    : m_db(nullptr)
    , m_meta(nullptr)
    , m_randomStream(nullptr)
    , m_error(false)
{
    m_xml.setAutoFormatting(true);
    m_xml.setAutoFormattingIndent(-1); // 1 tab
    m_xml.setCodec("UTF-8");
}

void KeePass2XmlWriter::writeDatabase(QIODevice* device, Database* db, KeePass2RandomStream* randomStream,
                                      const QByteArray& headerHash)
{
    m_db = db;
    m_meta = db->metadata();
    m_randomStream = randomStream;
    m_headerHash = headerHash;

    generateIdMap();

    m_xml.setDevice(device);

    m_xml.writeStartDocument("1.0", true);

    m_xml.writeStartElement("KeePassFile");

    writeMetadata();
    writeRoot();

    m_xml.writeEndElement();

    m_xml.writeEndDocument();

    if (m_xml.hasError()) {
        raiseError(device->errorString());
    }
}

void KeePass2XmlWriter::writeDatabase(const QString& filename, Database* db)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly|QIODevice::Truncate);
    writeDatabase(&file, db);
}

bool KeePass2XmlWriter::hasError()
{
    return m_error;
}

QString KeePass2XmlWriter::errorString()
{
    return m_errorStr;
}

void KeePass2XmlWriter::generateIdMap()
{
    QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(true);
    int nextId = 0;

    Q_FOREACH (Entry* entry, allEntries) {
        Q_FOREACH (const QString& key, entry->attachments()->keys()) {
            QByteArray data = entry->attachments()->value(key);
            if (!m_idMap.contains(data)) {
                m_idMap.insert(data, nextId++);
            }
        }
    }
}

void KeePass2XmlWriter::writeMetadata()
{
    m_xml.writeStartElement("Meta");

    writeString("Generator", m_meta->generator());
    if (!m_headerHash.isEmpty()) {
        writeBinary("HeaderHash", m_headerHash);
    }
    writeString("DatabaseName", m_meta->name());
    writeDateTime("DatabaseNameChanged", m_meta->nameChanged());
    writeString("DatabaseDescription", m_meta->description());
    writeDateTime("DatabaseDescriptionChanged", m_meta->descriptionChanged());
    writeString("DefaultUserName", m_meta->defaultUserName());
    writeDateTime("DefaultUserNameChanged", m_meta->defaultUserNameChanged());
    writeNumber("MaintenanceHistoryDays", m_meta->maintenanceHistoryDays());
    writeColor("Color", m_meta->color());
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
    writeNumber("HistoryMaxItems", m_meta->historyMaxItems());
    writeNumber("HistoryMaxSize", m_meta->historyMaxSize());
    writeBinaries();
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
    // writeBool("AutoEnableVisualHiding", m_meta->autoEnableVisualHiding());

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeCustomIcons()
{
    m_xml.writeStartElement("CustomIcons");

    Q_FOREACH (const Uuid& uuid, m_meta->customIconsOrder()) {
        writeIcon(uuid, m_meta->customIcon(uuid));
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeIcon(const Uuid& uuid, const QImage& icon)
{
    m_xml.writeStartElement("Icon");

    writeUuid("UUID", uuid);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    // TODO: check !icon.save()
    icon.save(&buffer, "PNG");
    buffer.close();
    writeBinary("Data", ba);

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeBinaries()
{
    m_xml.writeStartElement("Binaries");

    QHash<QByteArray, int>::const_iterator i;
    for (i = m_idMap.constBegin(); i != m_idMap.constEnd(); ++i) {
        m_xml.writeStartElement("Binary");

        m_xml.writeAttribute("ID", QString::number(i.value()));

        QByteArray data;
        if (m_db->compressionAlgo() == Database::CompressionGZip) {
            m_xml.writeAttribute("Compressed", "True");

            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);

            QtIOCompressor compressor(&buffer);
            compressor.setStreamFormat(QtIOCompressor::GzipFormat);
            compressor.open(QIODevice::WriteOnly);

            qint64 bytesWritten = compressor.write(i.key());
            Q_ASSERT(bytesWritten == i.key().size());
            Q_UNUSED(bytesWritten);
            compressor.close();

            buffer.seek(0);
            data = buffer.readAll();
        }
        else {
            data = i.key();
        }

        if (!data.isEmpty()) {
            m_xml.writeCharacters(QString::fromLatin1(data.toBase64()));
        }
        m_xml.writeEndElement();
    }

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
    Q_ASSERT(!group->uuid().isNull());

    m_xml.writeStartElement("Group");

    writeUuid("UUID", group->uuid());
    writeString("Name", group->name());
    writeString("Notes", group->notes());
    writeNumber("IconID", group->iconNumber());

    if (!group->iconUuid().isNull()) {
        writeUuid("CustomIconUUID", group->iconUuid());
    }
    writeTimes(group->timeInfo());
    writeBool("IsExpanded", group->isExpanded());
    writeString("DefaultAutoTypeSequence", group->defaultAutoTypeSequence());

    writeTriState("EnableAutoType", group->autoTypeEnabled());

    writeTriState("EnableSearching", group->searchingEnabled());

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
    Q_ASSERT(!entry->uuid().isNull());

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

    Q_FOREACH (const QString& key, entry->attributes()->keys()) {
        m_xml.writeStartElement("String");

        bool protect = ( ((key == "Title") && m_meta->protectTitle()) ||
                         ((key == "UserName") && m_meta->protectUsername()) ||
                         ((key == "Password") && m_meta->protectPassword()) ||
                         ((key == "URL") && m_meta->protectUrl()) ||
                         ((key == "Notes") && m_meta->protectNotes()) ||
                         entry->attributes()->isProtected(key) );

        writeString("Key", key);

        m_xml.writeStartElement("Value");
        QString value;

        if (protect) {
            if (m_randomStream) {
                m_xml.writeAttribute("Protected", "True");
                bool ok;
                QByteArray rawData = m_randomStream->process(entry->attributes()->value(key).toUtf8(), &ok);
                if (!ok) {
                    raiseError(m_randomStream->errorString());
                }
                value = QString::fromLatin1(rawData.toBase64());
            }
            else {
                m_xml.writeAttribute("ProtectInMemory", "True");
                value = entry->attributes()->value(key);
            }
        }
        else {
            value = entry->attributes()->value(key);
        }

        if (!value.isEmpty()) {
            m_xml.writeCharacters(stripInvalidXml10Chars(value));
        }
        m_xml.writeEndElement();

        m_xml.writeEndElement();
    }

    Q_FOREACH (const QString& key, entry->attachments()->keys()) {
        m_xml.writeStartElement("Binary");

        writeString("Key", key);

        m_xml.writeStartElement("Value");
        m_xml.writeAttribute("Ref", QString::number(m_idMap[entry->attachments()->value(key)]));
        m_xml.writeEndElement();

        m_xml.writeEndElement();
    }

    writeAutoType(entry);
    // write history only for entries that are not history items
    if (entry->parent()) {
        writeEntryHistory(entry);
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeAutoType(const Entry* entry)
{
    m_xml.writeStartElement("AutoType");

    writeBool("Enabled", entry->autoTypeEnabled());
    writeNumber("DataTransferObfuscation", entry->autoTypeObfuscation());
    writeString("DefaultSequence", entry->defaultAutoTypeSequence());

    Q_FOREACH (const AutoTypeAssociations::Association& assoc, entry->autoTypeAssociations()->getAll()) {
        writeAutoTypeAssoc(assoc);
    }

    m_xml.writeEndElement();
}

void KeePass2XmlWriter::writeAutoTypeAssoc(const AutoTypeAssociations::Association& assoc)
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
    if (string.isEmpty()) {
        m_xml.writeEmptyElement(qualifiedName);
    }
    else {
        m_xml.writeTextElement(qualifiedName, stripInvalidXml10Chars(string));
    }
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
    Q_ASSERT(dateTime.timeSpec() == Qt::UTC);

    QString dateTimeStr = dateTime.toString(Qt::ISODate);

    // Qt < 4.8 doesn't append a 'Z' at the end
    if (!dateTimeStr.isEmpty() && dateTimeStr[dateTimeStr.size() - 1] != 'Z') {
        dateTimeStr.append('Z');
    }

    writeString(qualifiedName, dateTimeStr);
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
    writeString(qualifiedName, QString::fromLatin1(ba.toBase64()));
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

void KeePass2XmlWriter::writeTriState(const QString& qualifiedName, Group::TriState triState)
{
    QString value;

    if (triState == Group::Inherit) {
        value = "null";
    }
    else if (triState == Group::Enable) {
        value = "true";
    }
    else {
        value = "false";
    }

    writeString(qualifiedName, value);
}

QString KeePass2XmlWriter::colorPartToString(int value)
{
    QString str = QString::number(value, 16).toUpper();
    if (str.length() == 1) {
        str.prepend("0");
    }

    return str;
}

QString KeePass2XmlWriter::stripInvalidXml10Chars(QString str)
{
    for (int i = str.size() - 1; i >= 0; i--) {
        const QChar ch = str.at(i);
        const ushort uc = ch.unicode();

        if (ch.isLowSurrogate() && i != 0 && str.at(i - 1).isHighSurrogate()) {
            // keep valid surrogate pair
            i--;
        }
        else if ((uc < 0x20 && uc != 0x09 && uc != 0x0A && uc != 0x0D)  // control chracters
                 || (uc >= 0x7F && uc <= 0x84)  // control chracters, valid but discouraged by XML
                 || (uc >= 0x86 && uc <= 0x9F)  // control chracters, valid but discouraged by XML
                 || (uc > 0xFFFD)               // noncharacter
                 || ch.isLowSurrogate()         // single low surrogate
                 || ch.isHighSurrogate())       // single high surrogate
        {
            qWarning("Stripping invalid XML 1.0 codepoint %x", uc);
            str.remove(i, 1);
        }
    }

    return str;
}

void KeePass2XmlWriter::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}
