/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "OPUXReader.h"

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Totp.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QUrl>

#include <minizip/unzip.h>

namespace
{
    QByteArray extractFile(unzFile uf, QString filename)
    {
        if (unzLocateFile(uf, filename.toLatin1(), 2) != UNZ_OK) {
            qWarning("Failed to extract 1PUX document: %s", qPrintable(filename));
            return {};
        }

        // Read export.data into memory
        int bytes, bytesRead = 0;
        QByteArray data;
        unzOpenCurrentFile(uf);
        do {
            data.resize(data.size() + 8192);
            bytes = unzReadCurrentFile(uf, data.data() + bytesRead, 8192);
            if (bytes > 0) {
                bytesRead += bytes;
            }
        } while (bytes > 0);
        unzCloseCurrentFile(uf);
        data.truncate(bytesRead);

        return data;
    }

    Entry* readItem(const QJsonObject& item, unzFile uf = nullptr)
    {
        const auto itemMap = item.toVariantMap();
        const auto overviewMap = itemMap.value("overview").toMap();
        const auto detailsMap = itemMap.value("details").toMap();

        // Create entry and assign basic values
        QScopedPointer<Entry> entry(new Entry());
        entry->setUuid(QUuid::createUuid());
        entry->setTitle(overviewMap.value("title").toString());
        entry->setUrl(overviewMap.value("url").toString());
        if (overviewMap.contains("urls")) {
            int i = 1;
            for (const auto& urlRaw : overviewMap.value("urls").toList()) {
                const auto urlMap = urlRaw.toMap();
                const auto url = urlMap.value("url").toString();
                if (entry->url() != url) {
                    entry->attributes()->set(
                        QString("%1_%2").arg(EntryAttributes::AdditionalUrlAttribute, QString::number(i)), url);
                    ++i;
                }
            }
        }
        if (overviewMap.contains("tags")) {
            entry->setTags(overviewMap.value("tags").toStringList().join(","));
        }
        if (itemMap.value("favIndex").toString() == "1") {
            entry->addTag(QObject::tr("Favorite", "Tag for favorite entries"));
        }
        if (itemMap.value("state").toString() == "archived") {
            entry->addTag(QObject::tr("Archived", "Tag for archived entries"));
        }

        // Parse the details map by setting the username, password, and notes first
        const auto loginFields = detailsMap.value("loginFields").toList();
        for (const auto& field : loginFields) {
            const auto fieldMap = field.toMap();
            const auto designation = fieldMap.value("designation").toString();
            if (designation.compare("username", Qt::CaseInsensitive) == 0) {
                entry->setUsername(fieldMap.value("value").toString());
            } else if (designation.compare("password", Qt::CaseInsensitive) == 0) {
                entry->setPassword(fieldMap.value("value").toString());
            }
        }
        entry->setNotes(detailsMap.value("notesPlain").toString());

        // Dive into the item sections to pull out advanced attributes
        const auto sections = detailsMap.value("sections").toList();
        for (const auto& section : sections) {
            // Derive a prefix for attribute names using the title or uuid if missing
            const auto sectionMap = section.toMap();
            auto prefix = sectionMap.value("title").toString();
            if (prefix.isEmpty()) {
                prefix = QUuid::createUuid().toString().mid(1, 5);
            }

            for (const auto& field : sectionMap.value("fields").toList()) {
                // Form the name of the attribute using the prefix and title or id
                const auto fieldMap = field.toMap();
                auto name = fieldMap.value("title").toString();
                if (name.isEmpty()) {
                    name = fieldMap.value("id").toString();
                }
                name = QString("%1_%2").arg(prefix, name);

                const auto valueMap = fieldMap.value("value").toMap();
                const auto key = valueMap.firstKey();
                if (key == "totp") {
                    // Build otpauth url
                    QUrl otpurl(QString("otpauth://totp/%1:%2?secret=%3")
                                    .arg(entry->title(), entry->username(), valueMap.value(key).toString()));

                    if (entry->hasTotp()) {
                        // Store multiple TOTP definitions as additional otp attributes
                        int i = 0;
                        name = "otp";
                        const auto attributes = entry->attributes()->keys();
                        while (attributes.contains(name)) {
                            name = QString("otp_%1").arg(++i);
                        }
                        entry->attributes()->set(name, otpurl.toEncoded(), true);
                    } else {
                        // First otp value encountered gets formal storage
                        entry->setTotp(Totp::parseSettings(otpurl.toEncoded()));
                    }
                } else if (key == "file") {
                    // Add a file to the entry attachments
                    const auto fileMap = valueMap.value(key).toMap();
                    const auto fileName = fileMap.value("fileName").toString();
                    const auto docId = fileMap.value("documentId").toString();
                    const auto data = extractFile(uf, QString("files/%1__%2").arg(docId, fileName));
                    if (!data.isNull()) {
                        entry->attachments()->set(fileName, data);
                    }
                } else {
                    auto value = valueMap.value(key).toString();
                    if (key == "date") {
                        // Convert date fields from Unix time
                        value = QDateTime::fromSecsSinceEpoch(valueMap.value(key).toULongLong(), Qt::UTC).toString();
                    } else if (key == "email") {
                        // Email address is buried in a sub-value
                        value = valueMap.value(key).toMap().value("email_address").toString();
                    } else if (key == "address") {
                        // Combine all the address attributes into a fully formed structure
                        const auto address = valueMap.value(key).toMap();
                        value = address.value("street").toString() + "\n" + address.value("city").toString() + ", "
                                + address.value("state").toString() + " " + address.value("zip").toString() + "\n"
                                + address.value("country").toString();
                    }

                    if (!value.isEmpty()) {
                        entry->attributes()->set(name, value, key == "concealed");
                    }
                }
            }
        }

        // Add a document attachment if defined
        if (detailsMap.contains("documentAttributes")) {
            const auto document = detailsMap.value("documentAttributes").toMap();
            const auto fileName = document.value("fileName").toString();
            const auto docId = document.value("documentId").toString();
            const auto data = extractFile(uf, QString("files/%1__%2").arg(docId, fileName));
            if (!data.isNull()) {
                entry->attachments()->set(fileName, data);
            }
        }

        // Collapse any accumulated history
        entry->removeHistoryItems(entry->historyItems());

        // Adjust the created and modified times
        auto timeInfo = entry->timeInfo();
        const auto createdTime = QDateTime::fromSecsSinceEpoch(itemMap.value("createdAt").toULongLong(), Qt::UTC);
        const auto modifiedTime = QDateTime::fromSecsSinceEpoch(itemMap.value("updatedAt").toULongLong(), Qt::UTC);
        timeInfo.setCreationTime(createdTime);
        timeInfo.setLastModificationTime(modifiedTime);
        timeInfo.setLastAccessTime(modifiedTime);
        entry->setTimeInfo(timeInfo);

        return entry.take();
    }

    void writeVaultToDatabase(const QJsonObject& vault, QSharedPointer<Database> db, unzFile uf = nullptr)
    {
        if (!vault.contains("attrs") || !vault.contains("items")) {
            // Early out if the vault is missing critical items
            return;
        }

        const auto attr = vault.value("attrs").toObject().toVariantMap();

        // Create group and assign basic values
        auto group = new Group();
        group->setUuid(QUuid::createUuid());
        group->setName(attr.value("name").toString());
        group->setParent(db->rootGroup());

        const auto items = vault.value("items").toArray();
        for (const auto& item : items) {
            auto entry = readItem(item.toObject(), uf);
            if (entry) {
                entry->setGroup(group, false);
            }
        }

        // Add the group icon if present
        const auto icon = attr.value("avatar").toString();
        if (!icon.isEmpty()) {
            auto data = extractFile(uf, QString("files/%1").arg(icon));
            if (!data.isNull()) {
                const auto uuid = QUuid::createUuid();
                db->metadata()->addCustomIcon(uuid, data);
                group->setIcon(uuid);
            }
        }
    }
} // namespace

bool OPUXReader::hasError()
{
    return !m_error.isEmpty();
}

QString OPUXReader::errorString()
{
    return m_error;
}

QSharedPointer<Database> OPUXReader::convert(const QString& path)
{
    m_error.clear();

    QFileInfo fileinfo(path);
    if (!fileinfo.exists()) {
        m_error = QObject::tr("File does not exist.").arg(path);
        return {};
    }

    // 1PUX is a zip file format, open it and process the contents in memory
    auto uf = unzOpen64(fileinfo.absoluteFilePath().toLatin1().constData());
    if (!uf) {
        m_error = QObject::tr("Invalid 1PUX file format: Not a valid ZIP file.");
        return {};
    }

    // Find the export.data file, if not found this isn't a 1PUX file
    auto data = extractFile(uf, "export.data");
    if (data.isNull()) {
        m_error = QObject::tr("Invalid 1PUX file format: Missing export.data");
        unzClose(uf);
        return {};
    }

    auto db = QSharedPointer<Database>::create();
    db->rootGroup()->setName(QObject::tr("1Password Import"));
    const auto json = QJsonDocument::fromJson(data);

    const auto account = json.object().value("accounts").toArray().first().toObject();
    const auto vaults = account.value("vaults").toArray();

    for (const auto& vault : vaults) {
        writeVaultToDatabase(vault.toObject(), db, uf);
    }

    unzClose(uf);
    return db;
}
