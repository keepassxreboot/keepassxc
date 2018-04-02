/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "KeeAgentSettings.h"

KeeAgentSettings::KeeAgentSettings()
    : m_allowUseOfSshKey(false)
    , m_addAtDatabaseOpen(false)
    , m_removeAtDatabaseClose(false)
    , m_useConfirmConstraintWhenAdding(false)
    , m_useLifetimeConstraintWhenAdding(false)
    , m_lifetimeConstraintDuration(600)
    , m_selectedType(QString("file"))
    , m_attachmentName(QString())
    , m_saveAttachmentToTempFile(false)
    , m_fileName(QString())
{
}

bool KeeAgentSettings::operator==(KeeAgentSettings& other)
{
    return (m_allowUseOfSshKey == other.m_allowUseOfSshKey && m_addAtDatabaseOpen == other.m_addAtDatabaseOpen
            && m_removeAtDatabaseClose == other.m_removeAtDatabaseClose
            && m_useConfirmConstraintWhenAdding == other.m_useConfirmConstraintWhenAdding
            && m_useLifetimeConstraintWhenAdding == other.m_useLifetimeConstraintWhenAdding
            && m_lifetimeConstraintDuration == other.m_lifetimeConstraintDuration
            && m_selectedType == other.m_selectedType
            && m_attachmentName == other.m_attachmentName
            && m_saveAttachmentToTempFile == other.m_saveAttachmentToTempFile
            && m_fileName == other.m_fileName);
}

bool KeeAgentSettings::operator!=(KeeAgentSettings& other)
{
    return !(*this == other);
}

bool KeeAgentSettings::isDefault()
{
    KeeAgentSettings defaultSettings;
    return (*this == defaultSettings);
}

bool KeeAgentSettings::allowUseOfSshKey() const
{
    return m_allowUseOfSshKey;
}

bool KeeAgentSettings::addAtDatabaseOpen() const
{
    return m_addAtDatabaseOpen;
}

bool KeeAgentSettings::removeAtDatabaseClose() const
{
    return m_removeAtDatabaseClose;
}

bool KeeAgentSettings::useConfirmConstraintWhenAdding() const
{
    return m_useConfirmConstraintWhenAdding;
}

bool KeeAgentSettings::useLifetimeConstraintWhenAdding() const
{
    return m_useLifetimeConstraintWhenAdding;
}

int KeeAgentSettings::lifetimeConstraintDuration() const
{
    return m_lifetimeConstraintDuration;
}

const QString KeeAgentSettings::selectedType() const
{
    return m_selectedType;
}

const QString KeeAgentSettings::attachmentName() const
{
    return m_attachmentName;
}

bool KeeAgentSettings::saveAttachmentToTempFile() const
{
    return m_saveAttachmentToTempFile;
}

const QString KeeAgentSettings::fileName() const
{
    return m_fileName;
}

void KeeAgentSettings::setAllowUseOfSshKey(bool allowUseOfSshKey)
{
    m_allowUseOfSshKey = allowUseOfSshKey;
}

void KeeAgentSettings::setAddAtDatabaseOpen(bool addAtDatabaseOpen)
{
    m_addAtDatabaseOpen = addAtDatabaseOpen;
}

void KeeAgentSettings::setRemoveAtDatabaseClose(bool removeAtDatabaseClose)
{
    m_removeAtDatabaseClose = removeAtDatabaseClose;
}

void KeeAgentSettings::setUseConfirmConstraintWhenAdding(bool useConfirmConstraintWhenAdding)
{
    m_useConfirmConstraintWhenAdding = useConfirmConstraintWhenAdding;
}

void KeeAgentSettings::setUseLifetimeConstraintWhenAdding(bool useLifetimeConstraintWhenAdding)
{
    m_useLifetimeConstraintWhenAdding = useLifetimeConstraintWhenAdding;
}

void KeeAgentSettings::setLifetimeConstraintDuration(int lifetimeConstraintDuration)
{
    m_lifetimeConstraintDuration = lifetimeConstraintDuration;
}

void KeeAgentSettings::setSelectedType(const QString& selectedType)
{
    m_selectedType = selectedType;
}

void KeeAgentSettings::setAttachmentName(const QString& attachmentName)
{
    m_attachmentName = attachmentName;
}

void KeeAgentSettings::setSaveAttachmentToTempFile(bool saveAttachmentToTempFile)
{
    m_saveAttachmentToTempFile = saveAttachmentToTempFile;
}

void KeeAgentSettings::setFileName(const QString& fileName)
{
    m_fileName = fileName;
}

bool KeeAgentSettings::readBool(QXmlStreamReader& reader)
{
    reader.readNext();
    bool ret = (reader.text().startsWith("t", Qt::CaseInsensitive));
    reader.readNext(); // tag end
    return ret;
}

int KeeAgentSettings::readInt(QXmlStreamReader& reader)
{
    reader.readNext();
    int ret = reader.text().toInt();
    reader.readNext(); // tag end
    return ret;
}

bool KeeAgentSettings::fromXml(const QByteArray& ba)
{
    QXmlStreamReader reader;
    reader.addData(ba);

    if (reader.error() || !reader.readNextStartElement()) {
        return false;
    }

    if (reader.qualifiedName() != "EntrySettings") {
        return false;
    }

    while (!reader.error() && reader.readNextStartElement()) {
        if (reader.name() == "AllowUseOfSshKey") {
            m_allowUseOfSshKey = readBool(reader);
        } else if (reader.name() == "AddAtDatabaseOpen") {
            m_addAtDatabaseOpen = readBool(reader);
        } else if (reader.name() == "RemoveAtDatabaseClose") {
            m_removeAtDatabaseClose = readBool(reader);
        } else if (reader.name() == "UseConfirmConstraintWhenAdding") {
            m_useConfirmConstraintWhenAdding = readBool(reader);
        } else if (reader.name() == "UseLifetimeConstraintWhenAdding") {
            m_useLifetimeConstraintWhenAdding = readBool(reader);
        } else if (reader.name() == "LifetimeConstraintDuration") {
            m_lifetimeConstraintDuration = readInt(reader);
        } else if (reader.name() == "Location") {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "SelectedType") {
                    reader.readNext();
                    m_selectedType = reader.text().toString();
                    reader.readNext();
                } else if (reader.name() == "AttachmentName") {
                    reader.readNext();
                    m_attachmentName = reader.text().toString();
                    reader.readNext();
                } else if (reader.name() == "SaveAttachmentToTempFile") {
                    m_saveAttachmentToTempFile = readBool(reader);
                } else if (reader.name() == "FileName") {
                    reader.readNext();
                    m_fileName = reader.text().toString();
                    reader.readNext();
                } else {
                    qWarning() << "Skipping location element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        } else {
            qWarning() << "Skipping element" << reader.name();
            reader.skipCurrentElement();
        }
    }

    return true;
}

QByteArray KeeAgentSettings::toXml()
{
    QByteArray ba;
    QXmlStreamWriter writer(&ba);

    // real KeeAgent can only read UTF-16
    writer.setCodec(QTextCodec::codecForName("UTF-16"));
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument();

    writer.writeStartElement("EntrySettings");
    writer.writeAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");

    writer.writeTextElement("AllowUseOfSshKey", m_allowUseOfSshKey ? "true" : "false");
    writer.writeTextElement("AddAtDatabaseOpen", m_addAtDatabaseOpen ? "true" : "false");
    writer.writeTextElement("RemoveAtDatabaseClose", m_removeAtDatabaseClose ? "true" : "false");
    writer.writeTextElement("UseConfirmConstraintWhenAdding", m_useConfirmConstraintWhenAdding ? "true" : "false");
    writer.writeTextElement("UseLifetimeConstraintWhenAdding", m_useLifetimeConstraintWhenAdding ? "true" : "false");
    writer.writeTextElement("LifetimeConstraintDuration", QString::number(m_lifetimeConstraintDuration));

    writer.writeStartElement("Location");
    writer.writeTextElement("SelectedType", m_selectedType);

    if (!m_attachmentName.isEmpty()) {
        writer.writeTextElement("AttachmentName", m_attachmentName);
    } else {
        writer.writeEmptyElement("AttachmentName");
    }

    writer.writeTextElement("SaveAttachmentToTempFile", m_saveAttachmentToTempFile ? "true" : "false");

    if (!m_fileName.isEmpty()) {
        writer.writeTextElement("FileName", m_fileName);
    } else {
        writer.writeEmptyElement("FileName");
    }

    writer.writeEndElement(); // Location
    writer.writeEndElement(); // EntrySettings
    writer.writeEndDocument();

    return ba;
}
