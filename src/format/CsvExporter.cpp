/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#include "CsvExporter.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSaveFile>

#include "core/AutoTypeAssociations.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"

namespace
{
    // columns after "Created" were appended so that the original ten keep their positions.
    // Structured values are JSON (compact) inside one CSV cell; an entry without such data leaves the cell empty.

    QString toJson(const QJsonObject& object)
    {
        return object.isEmpty() ? QString() : QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
    }

    QString toJson(const QJsonArray& array)
    {
        return array.isEmpty() ? QString() : QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    QString isoTime(const QDateTime& time)
    {
        return time.toString(Qt::ISODate);
    }

    QJsonObject customAttributes(const Entry* entry)
    {
        QJsonObject attributes;
        const auto* entryAttributes = entry->attributes();
        for (const auto& key : entryAttributes->customKeys()) {
            attributes.insert(key, entryAttributes->value(key));
        }
        return attributes;
    }

    QJsonObject autoTypeSettings(const Entry* entry)
    {
        QJsonArray associations;
        for (const auto& association : entry->autoTypeAssociations()->getAll()) {
            associations.append(QJsonObject{{"window", association.window}, {"sequence", association.sequence}});
        }

        // Only exported when it differs from the defaults (enabled, no obfuscation, default sequence, no windows)
        if (entry->autoTypeEnabled() && entry->autoTypeObfuscation() == 0 && entry->defaultAutoTypeSequence().isEmpty()
            && associations.isEmpty()) {
            return {};
        }

        QJsonObject settings;
        settings.insert("enabled", entry->autoTypeEnabled());
        settings.insert("obfuscation", entry->autoTypeObfuscation());
        settings.insert("defaultSequence", entry->defaultAutoTypeSequence());
        settings.insert("associations", associations);
        return settings;
    }

    QJsonObject attachments(const Entry* entry)
    {
        QJsonObject result;
        const auto* entryAttachments = entry->attachments();
        for (const auto& key : entryAttachments->keys()) {
            result.insert(key, QString::fromLatin1(entryAttachments->value(key).toBase64()));
        }
        return result;
    }

    QJsonArray history(const Entry* entry)
    {
        QJsonArray snapshots;
        for (const auto* item : entry->historyItems()) {
            QJsonObject snapshot;
            snapshot.insert("title", item->title());
            snapshot.insert("username", item->username());
            snapshot.insert("password", item->password());
            snapshot.insert("url", item->url());
            snapshot.insert("notes", item->notes());
            snapshot.insert("modified", isoTime(item->timeInfo().lastModificationTime()));
            const auto attributes = customAttributes(item);
            if (!attributes.isEmpty()) {
                snapshot.insert("attributes", attributes);
            }
            snapshots.append(snapshot);
        }
        return snapshots;
    }
} // namespace

bool CsvExporter::exportDatabase(const QString& filename, const QSharedPointer<const Database>& db)
{
    // write to a temporary file and replace the target only once everything was written, so a crash,
    // a full disk or an interruption never leaves a truncated export in place of the previous file.
    QSaveFile file(filename);
    file.setDirectWriteFallback(true);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = file.errorString();
        return false;
    }

    if (!exportDatabase(&file, db)) {
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        m_error = file.errorString();
        return false;
    }
    return true;
}

bool CsvExporter::exportDatabase(QIODevice* device, const QSharedPointer<const Database>& db)
{
    if (!writeText(device, exportHeader())) {
        return false;
    }

    // stream entry by entry; attachments are exported too and can be large
    return writeGroup(device, db->rootGroup(), QString());
}

QString CsvExporter::exportDatabase(const QSharedPointer<const Database>& db)
{
    return exportHeader() + exportGroup(db->rootGroup());
}

QString CsvExporter::errorString() const
{
    return m_error;
}

bool CsvExporter::writeText(QIODevice* device, const QString& text)
{
    const auto data = text.toUtf8();
    // a short write is a failure too, not only -1
    if (device->write(data) != data.size()) {
        m_error = device->errorString();
        if (m_error.isEmpty()) {
            m_error = QObject::tr("Not all data could be written.");
        }
        return false;
    }
    return true;
}

QString CsvExporter::exportHeader()
{
    QString header;
    addColumn(header, "Group");
    addColumn(header, "Title");
    addColumn(header, "Username");
    addColumn(header, "Password");
    addColumn(header, "URL");
    addColumn(header, "Notes");
    addColumn(header, "TOTP");
    addColumn(header, "Icon");
    addColumn(header, "Last Modified");
    addColumn(header, "Created");
    // everything below was not exported before
    addColumn(header, "Tags");
    addColumn(header, "Expires");
    addColumn(header, "Expiry Time");
    addColumn(header, "Last Accessed");
    addColumn(header, "Location Changed");
    addColumn(header, "Usage Count");
    addColumn(header, "UUID");
    addColumn(header, "Custom Attributes");
    addColumn(header, "Auto-Type");
    addColumn(header, "Attachments");
    addColumn(header, "History");
    return header + QString("\n");
}

QString CsvExporter::exportEntry(const Entry* entry, const QString& groupPath)
{
    QString line;

    addColumn(line, groupPath);
    addColumn(line, entry->title());
    addColumn(line, entry->username());
    addColumn(line, entry->password());
    addColumn(line, entry->url());
    addColumn(line, entry->notes());
    addColumn(line, entry->totpSettingsString());
    addColumn(line, QString::number(entry->iconNumber()));
    addColumn(line, isoTime(entry->timeInfo().lastModificationTime()));
    addColumn(line, isoTime(entry->timeInfo().creationTime()));
    addColumn(line, entry->tags());
    addColumn(line, entry->timeInfo().expires() ? "true" : "false");
    addColumn(line, isoTime(entry->timeInfo().expiryTime()));
    addColumn(line, isoTime(entry->timeInfo().lastAccessTime()));
    addColumn(line, isoTime(entry->timeInfo().locationChanged()));
    addColumn(line, QString::number(entry->timeInfo().usageCount()));
    addColumn(line, entry->uuidToHex());
    addColumn(line, toJson(customAttributes(entry)));
    addColumn(line, toJson(autoTypeSettings(entry)));
    addColumn(line, toJson(attachments(entry)));
    addColumn(line, toJson(history(entry)));

    line.append("\n");
    return line;
}

QString CsvExporter::exportGroup(const Group* group, QString groupPath)
{
    QString response;
    if (!groupPath.isEmpty()) {
        groupPath.append("/");
    }
    groupPath.append(group->name());

    const QList<Entry*>& entryList = group->entries();
    for (const Entry* entry : entryList) {
        response.append(exportEntry(entry, groupPath));
    }

    const QList<Group*>& children = group->children();
    for (const Group* child : children) {
        response.append(exportGroup(child, groupPath));
    }

    return response;
}

bool CsvExporter::writeGroup(QIODevice* device, const Group* group, QString groupPath)
{
    if (!groupPath.isEmpty()) {
        groupPath.append("/");
    }
    groupPath.append(group->name());

    const QList<Entry*>& entryList = group->entries();
    for (const Entry* entry : entryList) {
        if (!writeText(device, exportEntry(entry, groupPath))) {
            return false;
        }
    }

    const QList<Group*>& children = group->children();
    for (const Group* child : children) {
        if (!writeGroup(device, child, groupPath)) {
            return false;
        }
    }

    return true;
}

void CsvExporter::addColumn(QString& str, const QString& column)
{
    if (!str.isEmpty()) {
        str.append(",");
    }

    str.append("\"");
    str.append(QString(column).replace("\"", "\"\""));
    str.append("\"");
}
