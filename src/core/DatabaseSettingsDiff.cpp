/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseSettingsDiff.h"

#include <QObject>
#include <QSet>

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"

namespace
{
    const QString CustomDataPrefix = QStringLiteral("customData:");

    // The values are shown the way a person reads them, never as raw internal numbers

    QString text(const QString& value)
    {
        return value.isEmpty() ? QObject::tr("(empty)") : value;
    }

    QString colour(const QString& value)
    {
        return value.isEmpty() ? QObject::tr("(none)") : value;
    }

    QString yesNo(bool value)
    {
        return value ? QObject::tr("Yes") : QObject::tr("No");
    }

    QString number(int value)
    {
        return value < 0 ? QObject::tr("No limit") : QString::number(value);
    }

    QString mebibytes(int bytes)
    {
        return bytes < 0 ? QObject::tr("No limit")
                         : QObject::tr("%1 MiB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
    }

    QString days(int value, const QString& negative)
    {
        return value < 0 ? negative : QObject::tr("%1 days").arg(value);
    }

    QString folder(const Group* group)
    {
        return group ? group->name() : QObject::tr("(none)");
    }

    QString shortened(const QString& value)
    {
        const int maxLength = 60;
        return value.size() > maxLength ? value.left(maxLength) + QChar(0x2026) : value;
    }

    QString customDataValue(const CustomData* data, const QString& key)
    {
        if (!data->contains(key)) {
            return QObject::tr("(not set)");
        }
        // Protected values (browser keys and the like) are never shown
        return data->isProtected(key) ? QObject::tr("(hidden)") : text(shortened(data->value(key)));
    }
} // namespace

namespace DatabaseSettingsDiff
{
    QList<DatabaseSettingChange> compare(const Database& open, const Database& imported)
    {
        QList<DatabaseSettingChange> changes;
        const auto* mine = open.metadata();
        const auto* theirs = imported.metadata();

        auto add = [&](const QString& id, const QString& label, const QString& current, const QString& file) {
            DatabaseSettingChange change;
            change.id = id;
            change.label = label;
            change.currentValue = current;
            change.fileValue = file;
            changes << change;
        };

        if (mine->name() != theirs->name()) {
            add("name", QObject::tr("Database name"), text(mine->name()), text(theirs->name()));
        }
        if (mine->description() != theirs->description()) {
            add("description",
                QObject::tr("Database description"),
                text(mine->description()),
                text(theirs->description()));
        }
        if (mine->defaultUserName() != theirs->defaultUserName()) {
            add("defaultUserName",
                QObject::tr("Default user name for new entries"),
                text(mine->defaultUserName()),
                text(theirs->defaultUserName()));
        }
        if (mine->color() != theirs->color()) {
            add("color", QObject::tr("Database colour"), colour(mine->color()), colour(theirs->color()));
        }
        if (mine->maintenanceHistoryDays() != theirs->maintenanceHistoryDays()) {
            add("maintenanceHistoryDays",
                QObject::tr("Maintenance history (days)"),
                days(mine->maintenanceHistoryDays(), QObject::tr("Never")),
                days(theirs->maintenanceHistoryDays(), QObject::tr("Never")));
        }
        if (mine->databaseKeyChangeRec() != theirs->databaseKeyChangeRec()) {
            add("masterKeyChangeRec",
                QObject::tr("Remind me to change the master key after"),
                days(mine->databaseKeyChangeRec(), QObject::tr("Never")),
                days(theirs->databaseKeyChangeRec(), QObject::tr("Never")));
        }
        if (mine->databaseKeyChangeForce() != theirs->databaseKeyChangeForce()) {
            add("masterKeyChangeForce",
                QObject::tr("Force a master key change after"),
                days(mine->databaseKeyChangeForce(), QObject::tr("Never")),
                days(theirs->databaseKeyChangeForce(), QObject::tr("Never")));
        }
        if (mine->protectTitle() != theirs->protectTitle()) {
            add("protectTitle",
                QObject::tr("Protect titles in memory"),
                yesNo(mine->protectTitle()),
                yesNo(theirs->protectTitle()));
        }
        if (mine->protectUsername() != theirs->protectUsername()) {
            add("protectUsername",
                QObject::tr("Protect user names in memory"),
                yesNo(mine->protectUsername()),
                yesNo(theirs->protectUsername()));
        }
        if (mine->protectPassword() != theirs->protectPassword()) {
            add("protectPassword",
                QObject::tr("Protect passwords in memory"),
                yesNo(mine->protectPassword()),
                yesNo(theirs->protectPassword()));
        }
        if (mine->protectUrl() != theirs->protectUrl()) {
            add("protectUrl",
                QObject::tr("Protect URLs in memory"),
                yesNo(mine->protectUrl()),
                yesNo(theirs->protectUrl()));
        }
        if (mine->protectNotes() != theirs->protectNotes()) {
            add("protectNotes",
                QObject::tr("Protect notes in memory"),
                yesNo(mine->protectNotes()),
                yesNo(theirs->protectNotes()));
        }
        if (mine->recycleBinEnabled() != theirs->recycleBinEnabled()) {
            add("recycleBinEnabled",
                QObject::tr("Use a recycle bin for deleted entries"),
                yesNo(mine->recycleBinEnabled()),
                yesNo(theirs->recycleBinEnabled()));
        }
        if (mine->historyMaxItems() != theirs->historyMaxItems()) {
            add("historyMaxItems",
                QObject::tr("Old versions kept per entry"),
                number(mine->historyMaxItems()),
                number(theirs->historyMaxItems()));
        }
        if (mine->historyMaxSize() != theirs->historyMaxSize()) {
            add("historyMaxSize",
                QObject::tr("Maximum size of the old versions of an entry"),
                mebibytes(mine->historyMaxSize()),
                mebibytes(theirs->historyMaxSize()));
        }

        // The folders are captured by UUID now: the imported tree is moved (and may be copied) before the choice is
        // applied, and the pointers of the imported database do not survive that.
        const auto* theirBin = theirs->recycleBin();
        if (theirBin && (!mine->recycleBin() || mine->recycleBin()->uuid() != theirBin->uuid())) {
            add("recycleBinGroup", QObject::tr("Recycle bin folder"), folder(mine->recycleBin()), folder(theirBin));
            changes.last().uuids << theirBin->uuid();
            changes.last().note =
                QObject::tr("The folder is added to your database together with the imported entries.");
        }
        const auto* theirTemplates = theirs->entryTemplatesGroup();
        if (theirTemplates
            && (!mine->entryTemplatesGroup() || mine->entryTemplatesGroup()->uuid() != theirTemplates->uuid())) {
            add("entryTemplatesGroup",
                QObject::tr("Folder for entry templates"),
                folder(mine->entryTemplatesGroup()),
                folder(theirTemplates));
            changes.last().uuids << theirTemplates->uuid();
            changes.last().note =
                QObject::tr("The folder is added to your database together with the imported entries.");
        }

        // Database custom data: browser connections, plugin data and the like. The program's own bookkeeping is
        // skipped.
        const auto* theirData = theirs->customData();
        const auto* myData = mine->customData();
        auto keys = theirData->keys();
        keys.sort();
        for (const auto& key : keys) {
            if (theirData->isAutoGenerated(key)) {
                continue;
            }
            if (myData->contains(key) && myData->value(key) == theirData->value(key)) {
                continue;
            }
            const bool browserKey = key.startsWith(CustomData::BrowserKeyPrefix);
            add(CustomDataPrefix + key,
                browserKey
                    ? QObject::tr("Browser extension connection: %1").arg(key.mid(CustomData::BrowserKeyPrefix.size()))
                    : QObject::tr("Database custom data: %1").arg(key),
                customDataValue(myData, key),
                customDataValue(theirData, key));
            if (browserKey) {
                changes.last().note =
                    QObject::tr("Carries over the trust link between this database and a browser extension.");
            } else if (theirData->isProtected(key)) {
                changes.last().note = QObject::tr("The value is protected, so it is not shown here.");
            }
        }

        // Icons of the file that nothing imported uses. The icons that are used travel with the entries and folders.
        const auto usedIcons = imported.rootGroup()->customIconsRecursive();
        QList<QUuid> unusedIcons;
        for (const auto& uuid : theirs->customIconsOrder()) {
            if (!usedIcons.contains(uuid) && !mine->hasCustomIcon(uuid)) {
                unusedIcons << uuid;
            }
        }
        if (!unusedIcons.isEmpty()) {
            add("unusedCustomIcons",
                QObject::tr("Custom icons that no imported entry or folder uses"),
                QObject::tr("(none)"),
                QObject::tr("%1 icons").arg(unusedIcons.size()));
            changes.last().uuids = unusedIcons;
        }

        // Records of things that were deleted
        int newDeletionRecords = 0;
        for (const auto& record : imported.deletedObjects()) {
            if (!open.containsDeletedObject(record)) {
                ++newDeletionRecords;
            }
        }
        if (newDeletionRecords > 0) {
            add("deletedObjects",
                QObject::tr("Records of deleted entries and folders"),
                QObject::tr("(none)"),
                QObject::tr("%1 records").arg(newDeletionRecords));
            changes.last().note = QObject::tr(
                "Applying these can delete entries or folders of your database that have the same ID the next time "
                "databases are merged.");
        }

        return changes;
    }

    QStringList apply(Database& open,
                      const Database& imported,
                      const QList<DatabaseSettingChange>& chosen,
                      const QHash<QUuid, Group*>& importedGroups)
    {
        QStringList notApplied;
        auto* meta = open.metadata();
        const auto* theirs = imported.metadata();

        for (const auto& change : chosen) {
            const auto& id = change.id;
            if (id == "name") {
                meta->setName(theirs->name());
            } else if (id == "description") {
                meta->setDescription(theirs->description());
            } else if (id == "defaultUserName") {
                meta->setDefaultUserName(theirs->defaultUserName());
            } else if (id == "color") {
                meta->setColor(theirs->color());
            } else if (id == "maintenanceHistoryDays") {
                meta->setMaintenanceHistoryDays(theirs->maintenanceHistoryDays());
            } else if (id == "masterKeyChangeRec") {
                meta->setMasterKeyChangeRec(theirs->databaseKeyChangeRec());
            } else if (id == "masterKeyChangeForce") {
                meta->setMasterKeyChangeForce(theirs->databaseKeyChangeForce());
            } else if (id == "protectTitle") {
                meta->setProtectTitle(theirs->protectTitle());
            } else if (id == "protectUsername") {
                meta->setProtectUsername(theirs->protectUsername());
            } else if (id == "protectPassword") {
                meta->setProtectPassword(theirs->protectPassword());
            } else if (id == "protectUrl") {
                meta->setProtectUrl(theirs->protectUrl());
            } else if (id == "protectNotes") {
                meta->setProtectNotes(theirs->protectNotes());
            } else if (id == "recycleBinEnabled") {
                meta->setRecycleBinEnabled(theirs->recycleBinEnabled());
            } else if (id == "historyMaxItems") {
                meta->setHistoryMaxItems(theirs->historyMaxItems());
            } else if (id == "historyMaxSize") {
                meta->setHistoryMaxSize(theirs->historyMaxSize());
            } else if (id == "recycleBinGroup" || id == "entryTemplatesGroup") {
                Group* target = change.uuids.isEmpty() ? nullptr : importedGroups.value(change.uuids.first());
                if (!target) {
                    notApplied << change.label;
                } else if (id == "recycleBinGroup") {
                    meta->setRecycleBin(target);
                } else {
                    meta->setEntryTemplatesGroup(target);
                }
            } else if (id.startsWith(CustomDataPrefix)) {
                const auto key = id.mid(CustomDataPrefix.size());
                if (theirs->customData()->contains(key)) {
                    meta->customData()->set(key, theirs->customData()->item(key));
                } else {
                    notApplied << change.label;
                }
            } else if (id == "unusedCustomIcons") {
                meta->copyCustomIcons(QSet<QUuid>(change.uuids.begin(), change.uuids.end()), theirs);
            } else if (id == "deletedObjects") {
                for (const auto& record : imported.deletedObjects()) {
                    if (!open.containsDeletedObject(record)) {
                        open.addDeletedObject(record);
                    }
                }
            } else {
                notApplied << change.label;
            }
        }

        return notApplied;
    }

    QHash<QUuid, Group*> matchGroups(const Group* original, Group* copy)
    {
        QHash<QUuid, Group*> result;
        // Group::clone keeps the order of the children, so both lists run in step
        const auto originals = original->groupsRecursive(true);
        const auto copies = copy->groupsRecursive(true);
        Q_ASSERT(originals.size() == copies.size());
        for (qsizetype i = 0; i < qMin(originals.size(), copies.size()); ++i) {
            result.insert(originals.at(i)->uuid(), copies.at(i));
        }
        return result;
    }

    QStringList notOffered()
    {
        return {
            QObject::tr("The name of the program that wrote the file"),
            QObject::tr("The date the master key of the file's database was last changed"),
            QObject::tr("Which folder was selected or visible last (screen position only)"),
            QObject::tr("The dates settings were changed (they are set again for the settings you apply)"),
        };
    }
} // namespace DatabaseSettingsDiff
