/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "Merger.h"

#include "core/Metadata.h"

namespace
{
    template <typename T> bool isNewer(const T* group_or_entry, const QDateTime& time)
    {
        return group_or_entry && group_or_entry->timeInfo().lastModificationTime() > time;
    }
    template <typename T> bool isOlder(const T* group_or_entry, const QDateTime& time)
    {
        return group_or_entry && time > group_or_entry->timeInfo().lastModificationTime();
    }

    bool hasAnyEntryOrChildModifiedAfterTime(const Group* group, const QDateTime& time)
    {
        if (!group) {
            return false;
        }
        for (const auto* child : group->children()) {
            Q_ASSERT(child);
            if (isNewer(child, time) || hasAnyEntryOrChildModifiedAfterTime(child, time)) {
                return true;
            }
        }
        for (const auto* entry : group->entriesRecursive(false)) {
            Q_ASSERT(entry);
            if (isNewer(entry, time)) {
                return true;
            }
        }
        return false;
    }

    template <typename T> const DeletedObject* searchDeletedObject(const Database* db, const T* group_or_entry)
    {
        if (!group_or_entry) {
            return nullptr;
        }
        const auto& deletedObjects = db->deletedObjects();
        auto deletion =
            std::find_if(deletedObjects.begin(), deletedObjects.end(), [group_or_entry](const DeletedObject& element) {
                return element.uuid == group_or_entry->uuid();
            });
        if (deletion != deletedObjects.end()) {
            return &*deletion;
        }
        return nullptr;
    }

    template <typename T>
    bool hasNewerParentDeletionThanTime(const Database* db, const T* group_or_entry, const QDateTime& time)
    {
        if (!group_or_entry) {
            return false;
        }

        if (const auto* deletion = searchDeletedObject(db, group_or_entry)) {
            return deletion->deletionTime > time;
        }
        if constexpr (std::is_same_v<T, Group>) {
            if (const auto* parentGroup = group_or_entry->parentGroup()) {
                return hasNewerParentDeletionThanTime(db, parentGroup, time);
            }
        } else if constexpr (std::is_same_v<T, Entry>) {
            if (const auto* group = group_or_entry->group()) {
                return hasNewerParentDeletionThanTime(db, group, time);
            }
        }
        return false;
    };
} // namespace

Merger::Change::Change(Type type, const Group& group, MergeOperation mergeOperation, QString details)
    : m_type{type}
    , m_group{group.fullPath()}
    , m_uuid{group.uuid()}
    , m_details{std::move(details)}
    , m_mergeOperation{std::move(mergeOperation)}
{
}
Merger::Change::Change(Type type, const Entry& entry, MergeOperation mergeOperation, QString details)
    : m_type{type}
    , m_title{entry.title()}
    , m_uuid{entry.uuid()}
    , m_details{std::move(details)}
    , m_mergeOperation{std::move(mergeOperation)}
{
    if (const auto* group = entry.group()) {
        m_group = group->fullPath();
    }
}
Merger::Change::Change(MergeOperation mergeOperation, QString details)
    : m_details{std::move(details)}
    , m_mergeOperation{std::move(mergeOperation)}
{
}

Merger::Change::Type Merger::Change::type() const
{
    return m_type;
}
const QString& Merger::Change::title() const
{
    return m_title;
}
const QString& Merger::Change::group() const
{
    return m_group;
}
const QUuid& Merger::Change::uuid() const
{
    return m_uuid;
}
const QString& Merger::Change::details() const
{
    return m_details;
}

QString Merger::Change::typeString() const
{
    switch (m_type) {
    case Type::Added:
        return tr("Added");
        break;
    case Type::Modified:
        return tr("Modified");
        break;
    case Type::Moved:
        return tr("Moved");
        break;
    case Type::Deleted:
        return tr("Deleted");
        break;
    case Type::Unspecified:
        return "";
        break;
    default:
        return "?";
    }
}

QString Merger::Change::toString() const
{
    QString result;
    if (m_type != Type::Unspecified) {
        result += QString("%1: ").arg(typeString());
    }
    if (!m_group.isEmpty()) {
        result += QString("'%1'").arg(m_group);
    }
    if (!m_title.isEmpty()) {
        result += QString("/'%1'").arg(m_title);
    }
    if (!m_uuid.isNull()) {
        result += QString("[%1]").arg(m_uuid.toString());
    }
    if (!m_details.isEmpty()) {
        result += QString("(%1)").arg(m_details);
    }
    return result;
}

void Merger::Change::merge()
{
    if (m_mergeOperation) {
        m_mergeOperation();
    }
}

Merger::Merger(const Database* sourceDb, Database* targetDb)
    : m_mode(Group::Default)
{
    if (!sourceDb || !targetDb) {
        Q_ASSERT(sourceDb && targetDb);
        return;
    }

    m_context = MergeContext{
        sourceDb, targetDb, sourceDb->rootGroup(), targetDb->rootGroup(), sourceDb->rootGroup(), targetDb->rootGroup()};
}

Merger::Merger(const Group* sourceGroup, Group* targetGroup)
    : m_mode(Group::Default)
{
    if (!sourceGroup || !targetGroup) { // TODO: can't the if and early return be removed?
        Q_ASSERT(sourceGroup && targetGroup);
        return;
    }

    m_context = MergeContext{sourceGroup->database(),
                             targetGroup->database(),
                             sourceGroup->database()->rootGroup(),
                             targetGroup->database()->rootGroup(),
                             sourceGroup,
                             targetGroup};
}

void Merger::setForcedMergeMode(Group::MergeMode mode)
{
    m_mode = mode;
}

void Merger::resetForcedMergeMode()
{
    m_mode = Group::Default;
}

Group::MergeMode Merger::mergeMode(const MergeContext& context) const
{
    if (m_mode != Group::MergeMode::Default) {
        return m_mode;
    }

    Q_ASSERT(context.m_targetGroup);
    return context.m_targetGroup->mergeMode();
}

const Merger::ChangeList& Merger::changes()
{
    auto changes = std::make_unique<ChangeList>();
    *changes << mergeGroup(m_context);
    *changes << mergeDeletions(m_context);
    *changes << mergeMetadata(m_context);
    m_changes = std::move(changes);
    return *m_changes;
}

const Merger::ChangeList& Merger::merge(bool recalculateChanges)
{
    if (recalculateChanges || !m_changes) {
        changes();
    }
    Q_ASSERT(m_changes);

    if (!m_changes->isEmpty()) {
        for (auto& change : *m_changes) {
            change.merge();
        }
        m_context.m_targetDb->markAsModified();
    }
    return *m_changes;
}

Merger::ChangeList Merger::mergeGroup(const MergeContext& context)
{
    ChangeList changes;

    auto hasNewerTargetDeletionThanModification = [&context](const auto* entry_or_group) {
        Q_ASSERT(entry_or_group);

        if (auto deletion = searchDeletedObject(context.m_targetDb, entry_or_group)) {
            return isOlder(entry_or_group, deletion->deletionTime);
        }
        return false;
    };
    auto hasTargetDeletion = [&context](const auto* entry_or_group) {
        return searchDeletedObject(context.m_targetDb, entry_or_group) != nullptr;
    };
    auto hasNewerParentTargetDeletionThanModification = [&context](const auto* group_or_entry) {
        Q_ASSERT(group_or_entry);

        return hasNewerParentDeletionThanTime(
            context.m_targetDb, group_or_entry, group_or_entry->timeInfo().lastModificationTime());
    };
    auto hasNewerChildModificationThanTargetDeletion = [&context](const Group* group) {
        Q_ASSERT(group);

        if (const auto* deletion = searchDeletedObject(context.m_targetDb, group)) {
            return hasAnyEntryOrChildModifiedAfterTime(group, deletion->deletionTime);
        }
        return false;
    };

    // merge entries
    const QList<Entry*> sourceEntries = context.m_sourceGroup->entries();
    for (Entry* sourceEntry : sourceEntries) {
        Q_ASSERT(sourceEntry);

        if (mergeMode(context) == Group::MergeMode::Synchronize
            && (hasNewerTargetDeletionThanModification(sourceEntry)
                || hasNewerParentTargetDeletionThanModification(sourceEntry))) {
            continue;
        }

        Entry* targetEntry = context.m_targetRootGroup->findEntryByUuid(sourceEntry->uuid());
        if (!targetEntry) {
            changes << Change{Change::Type::Added,
                              *sourceEntry,
                              [this, sourceEntry, context]() {
                                  //  This entry does not exist at all. Create it.
                                  auto* cloneEntry = sourceEntry->clone(Entry::CloneIncludeHistory);
                                  moveEntry(cloneEntry, context.m_targetGroup);
                              },
                              hasTargetDeletion(sourceEntry) ? tr("Restoring") : tr("Creating missing")};
        } else {
            // Entry is already present in the database. Update it.
            const bool locationChanged =
                targetEntry->timeInfo().locationChanged() < sourceEntry->timeInfo().locationChanged();
            if (locationChanged && targetEntry->group() != context.m_targetGroup) {
                changes << Change{Change::Type::Moved,
                                  *sourceEntry,
                                  [this, targetEntry, context]() {
                                      // TODO: wrap every pointer capture into QPointer?
                                      //       Could it be deleted in the meantime?
                                      moveEntry(targetEntry, context.m_targetGroup);
                                  },
                                  tr("Relocating")};
            }
            changes << resolveEntryConflict(context, sourceEntry, targetEntry);
        }
    }

    // merge groups recursively
    const QList<Group*> sourceChildGroups = context.m_sourceGroup->children();
    for (Group* sourceChildGroup : sourceChildGroups) {
        Q_ASSERT(sourceChildGroup);

        if (mergeMode(context) == Group::MergeMode::Synchronize
            && (hasNewerTargetDeletionThanModification(sourceChildGroup)
                || hasNewerParentTargetDeletionThanModification(sourceChildGroup))
            && !hasNewerChildModificationThanTargetDeletion(sourceChildGroup)) {
            continue;
        }

        Group* targetChildGroup = context.m_targetRootGroup->findGroupByUuid(sourceChildGroup->uuid());
        auto setLocationChanged = [sourceChildGroup](Group* group) {
            TimeInfo timeinfo = group->timeInfo();
            timeinfo.setLocationChanged(sourceChildGroup->timeInfo().locationChanged());
            group->setTimeInfo(timeinfo);
        };
        if (!targetChildGroup) {
            std::unique_ptr<Group> managedTargetChildGroup(
                sourceChildGroup->clone(Entry::CloneNoFlags, Group::CloneNoFlags));
            // object will be kept alive by lambda in Change object
            targetChildGroup = managedTargetChildGroup.get();
            auto movableFunctionWrapper = [](auto&& f) {
                // workaround for std::function to contain non-copyable lambda captures;
                // in C++23, std::move_only_function can be used instead
                using F = decltype(f);
                auto pf = std::make_shared<std::decay_t<F>>(std::forward<F>(f));
                return [pf]() { (*pf)(); };
            };
            changes << Change{Change::Type::Added,
                              *sourceChildGroup,
                              movableFunctionWrapper([this,
                                                      context,
                                                      targetChildGroup = std::move(managedTargetChildGroup),
                                                      setLocationChanged]() mutable {
                                  auto* releasedTargetChildGroup = targetChildGroup.release();
                                  moveGroup(releasedTargetChildGroup, context.m_targetGroup);
                                  setLocationChanged(releasedTargetChildGroup);
                              }),
                              hasTargetDeletion(sourceChildGroup) ? tr("Restoring") : tr("Creating missing")};
        } else {
            bool locationChanged =
                targetChildGroup->timeInfo().locationChanged() < sourceChildGroup->timeInfo().locationChanged();
            if (locationChanged && targetChildGroup->parent() != context.m_targetGroup) {
                changes << Change{Change::Type::Moved,
                                  *sourceChildGroup,
                                  [this, context, targetChildGroup, setLocationChanged]() {
                                      moveGroup(targetChildGroup, context.m_targetGroup);
                                      setLocationChanged(targetChildGroup);
                                  },
                                  tr("Relocating")};
            }
            changes << resolveGroupConflict(context, sourceChildGroup, targetChildGroup);
        }
        MergeContext subcontext{context.m_sourceDb,
                                context.m_targetDb,
                                context.m_sourceRootGroup,
                                context.m_targetRootGroup,
                                sourceChildGroup,
                                targetChildGroup};
        changes << mergeGroup(subcontext);
    }
    return changes;
}

Merger::ChangeList
Merger::resolveGroupConflict(const MergeContext& context, const Group* sourceChildGroup, Group* targetChildGroup)
{
    Q_UNUSED(context);
    ChangeList changes;

    const QDateTime timeExisting = targetChildGroup->timeInfo().lastModificationTime();
    const QDateTime timeOther = sourceChildGroup->timeInfo().lastModificationTime();

    // only if the other group is newer, update the existing one.
    if (timeExisting < timeOther) {
        changes << Change{Change::Type::Modified,
                          *sourceChildGroup,
                          [sourceChildGroup, targetChildGroup, timeOther]() {
                              targetChildGroup->setName(sourceChildGroup->name());
                              targetChildGroup->setNotes(sourceChildGroup->notes());
                              if (sourceChildGroup->iconNumber() == 0) {
                                  targetChildGroup->setIcon(sourceChildGroup->iconUuid());
                              } else {
                                  targetChildGroup->setIcon(sourceChildGroup->iconNumber());
                              }
                              targetChildGroup->setExpiryTime(sourceChildGroup->timeInfo().expiryTime());
                              TimeInfo timeInfo = targetChildGroup->timeInfo();
                              timeInfo.setLastModificationTime(timeOther);
                              targetChildGroup->setTimeInfo(timeInfo);
                          },
                          tr("Overwriting group data (name,notes,icon,expiry time,modification time)")};
    }
    return changes;
}

void Merger::moveEntry(Entry* entry, Group* targetGroup)
{
    Q_ASSERT(entry);
    Group* sourceGroup = entry->group();
    if (sourceGroup == targetGroup) {
        return;
    }
    const bool sourceGroupUpdateTimeInfo = sourceGroup ? sourceGroup->canUpdateTimeinfo() : false;
    if (sourceGroup) {
        sourceGroup->setUpdateTimeinfo(false);
    }
    const bool targetGroupUpdateTimeInfo = targetGroup ? targetGroup->canUpdateTimeinfo() : false;
    if (targetGroup) {
        targetGroup->setUpdateTimeinfo(false);
    }
    const bool entryUpdateTimeInfo = entry->canUpdateTimeinfo();
    entry->setUpdateTimeinfo(false);

    entry->setGroup(targetGroup);

    entry->setUpdateTimeinfo(entryUpdateTimeInfo);
    if (targetGroup) {
        targetGroup->setUpdateTimeinfo(targetGroupUpdateTimeInfo);
    }
    if (sourceGroup) {
        sourceGroup->setUpdateTimeinfo(sourceGroupUpdateTimeInfo);
    }
}

void Merger::moveGroup(Group* group, Group* targetGroup)
{
    Q_ASSERT(group);
    Group* sourceGroup = group->parentGroup();
    if (sourceGroup == targetGroup) {
        return;
    }
    const bool sourceGroupUpdateTimeInfo = sourceGroup ? sourceGroup->canUpdateTimeinfo() : false;
    if (sourceGroup) {
        sourceGroup->setUpdateTimeinfo(false);
    }
    const bool targetGroupUpdateTimeInfo = targetGroup ? targetGroup->canUpdateTimeinfo() : false;
    if (targetGroup) {
        targetGroup->setUpdateTimeinfo(false);
    }
    const bool groupUpdateTimeInfo = group->canUpdateTimeinfo();
    group->setUpdateTimeinfo(false);

    group->setParent(targetGroup);

    group->setUpdateTimeinfo(groupUpdateTimeInfo);
    if (targetGroup) {
        targetGroup->setUpdateTimeinfo(targetGroupUpdateTimeInfo);
    }
    if (sourceGroup) {
        sourceGroup->setUpdateTimeinfo(sourceGroupUpdateTimeInfo);
    }
}

void Merger::eraseEntry(Entry* entry)
{
    Database* database = entry->database();
    // most simple method to remove an item from DeletedObjects :(
    const QList<DeletedObject> deletions = database->deletedObjects();
    Group* parentGroup = entry->group();
    const bool groupUpdateTimeInfo = parentGroup ? parentGroup->canUpdateTimeinfo() : false;
    if (parentGroup) {
        parentGroup->setUpdateTimeinfo(false);
    }
    delete entry;
    if (parentGroup) {
        parentGroup->setUpdateTimeinfo(groupUpdateTimeInfo);
    }
    database->setDeletedObjects(deletions);
}

void Merger::eraseGroup(Group* group)
{
    Database* database = group->database();
    // most simple method to remove an item from DeletedObjects :(
    const QList<DeletedObject> deletions = database->deletedObjects();
    Group* parentGroup = group->parentGroup();
    const bool groupUpdateTimeInfo = parentGroup ? parentGroup->canUpdateTimeinfo() : false;
    if (parentGroup) {
        parentGroup->setUpdateTimeinfo(false);
    }
    delete group;
    if (parentGroup) {
        parentGroup->setUpdateTimeinfo(groupUpdateTimeInfo);
    }
    database->setDeletedObjects(deletions);
}

Merger::ChangeList Merger::resolveEntryConflict_MergeHistories(const MergeContext& context,
                                                               const Entry* sourceEntry,
                                                               Entry* targetEntry,
                                                               Group::MergeMode mergeMethod)
{
    Q_UNUSED(context);

    ChangeList changes;
    const int comparison = compare(targetEntry->timeInfo().lastModificationTime(),
                                   sourceEntry->timeInfo().lastModificationTime(),
                                   CompareItemIgnoreMilliseconds);
    const int maxItems = targetEntry->database()->metadata()->historyMaxItems();
    if (comparison < 0) {
        // TODO: this mergeHistory call before the actual merge is new;
        //       does it make sense?
        auto tmpEntry = QScopedPointer<Entry>{sourceEntry->clone(Entry::CloneIncludeHistory)};
        mergeHistory(targetEntry, tmpEntry.data(), mergeMethod, maxItems, &changes);
        changes << Change{Change::Type::Modified,
                          *targetEntry,
                          [this, sourceEntry, targetEntry, mergeMethod, maxItems]() {
                              Group* currentGroup = targetEntry->group();
                              Entry* clonedEntry = sourceEntry->clone(Entry::CloneIncludeHistory);
                              qDebug("Merge %s/%s with alien on top under %s",
                                     qPrintable(targetEntry->title()),
                                     qPrintable(sourceEntry->title()),
                                     qPrintable(currentGroup->name()));
                              mergeHistory(targetEntry, clonedEntry, mergeMethod, maxItems);
                              eraseEntry(targetEntry);
                              moveEntry(clonedEntry, currentGroup);
                          },
                          tr("Synchronizing from newer source")};
    } else {
        auto tmpEntry = QScopedPointer<Entry>{targetEntry->clone(Entry::CloneIncludeHistory)};
        const bool changed = mergeHistory(sourceEntry, tmpEntry.data(), mergeMethod, maxItems, &changes);
        if (changed) {
            changes << Change{Change::Type::Modified,
                              *targetEntry,
                              [this, sourceEntry, targetEntry, mergeMethod, maxItems]() {
                                  qDebug("Merge %s/%s with local on top/under %s",
                                         qPrintable(targetEntry->title()),
                                         qPrintable(sourceEntry->title()),
                                         qPrintable(targetEntry->group()->name()));
                                  mergeHistory(sourceEntry, targetEntry, mergeMethod, maxItems);
                              },
                              tr("Synchronizing from older source")};
        }
    }
    return changes;
}

Merger::ChangeList
Merger::resolveEntryConflict(const MergeContext& context, const Entry* sourceEntry, Entry* targetEntry)
{
    // TODO: is this comment at the correct place here?
    // We need to cut off the milliseconds since the persistent format only supports times down to seconds
    // so when we import data from a remote source, it may represent the (or even some msec newer) data
    // which may be discarded due to higher runtime precision

    Group::MergeMode mode = mergeMode(context);
    return resolveEntryConflict_MergeHistories(context, sourceEntry, targetEntry, mode);
}

bool Merger::mergeHistory(const Entry* sourceEntry,
                          Entry* targetEntry,
                          Group::MergeMode mergeMethod,
                          const int maxItems,
                          ChangeList* changes)
{
    Q_UNUSED(mergeMethod);
    const auto targetHistoryItems = targetEntry->historyItems();
    const auto sourceHistoryItems = sourceEntry->historyItems();
    const int comparison = compare(sourceEntry->timeInfo().lastModificationTime(),
                                   targetEntry->timeInfo().lastModificationTime(),
                                   CompareItemIgnoreMilliseconds);
    const bool preferLocal = comparison < 0;
    const bool preferRemote = comparison > 0;

    QMap<QDateTime, Entry*> merged;
    auto prettyTime = [](const QDateTime& time) { return time.toString("yyyy-MM-dd HH-mm-ss-zzz"); };
    for (Entry* historyItem : targetHistoryItems) {
        const QDateTime modificationTime = Clock::serialized(historyItem->timeInfo().lastModificationTime());
        if (merged.contains(modificationTime)
            && !merged[modificationTime]->equals(historyItem, CompareItemIgnoreMilliseconds)) {
            ::qWarning("Inconsistent history entry of %s[%s] at %s contains conflicting changes - conflict resolution "
                       "may lose data!",
                       qPrintable(sourceEntry->title()),
                       qPrintable(sourceEntry->uuidToHex()),
                       qPrintable(prettyTime(modificationTime)));
            if (changes) {
                *changes << Change{
                    Change::Type::Unspecified,
                    *sourceEntry,
                    // TODO: this might cause duplicate warning output; maybe just use empty merge operation?
                    []() {},
                    tr("Inconsistent history - may cause data loss (at '%1' in source database)!")
                        .arg(prettyTime(modificationTime))};
            }
        }
        merged[modificationTime] = historyItem->clone(Entry::CloneNoFlags);
    }
    for (Entry* historyItem : sourceHistoryItems) {
        // Items with same modification-time changes will be regarded as same (like KeePass2)
        const QDateTime modificationTime = Clock::serialized(historyItem->timeInfo().lastModificationTime());
        if (merged.contains(modificationTime)
            && !merged[modificationTime]->equals(historyItem, CompareItemIgnoreMilliseconds)) {
            ::qWarning(
                "History entry of %s[%s] at %s contains conflicting changes - conflict resolution may lose data!",
                qPrintable(sourceEntry->title()),
                qPrintable(sourceEntry->uuidToHex()),
                qPrintable(prettyTime(modificationTime)));
            if (changes) {
                *changes << Change{
                    Change::Type::Unspecified,
                    *sourceEntry,
                    []() {},
                    tr("History conflict - may cause data loss (at '%1')!").arg(prettyTime(modificationTime))};
            }
        }
        if (preferRemote && merged.contains(modificationTime)) {
            // forcefully apply the remote history item
            delete merged.take(modificationTime);
        }
        if (!merged.contains(modificationTime)) {
            merged[modificationTime] = historyItem->clone(Entry::CloneNoFlags);
        }
    }

    const QDateTime targetModificationTime = Clock::serialized(targetEntry->timeInfo().lastModificationTime());
    const QDateTime sourceModificationTime = Clock::serialized(sourceEntry->timeInfo().lastModificationTime());
    if (targetModificationTime == sourceModificationTime
        && !targetEntry->equals(sourceEntry,
                                CompareItemIgnoreMilliseconds | CompareItemIgnoreHistory | CompareItemIgnoreLocation)) {
        ::qWarning("Entry of %s[%s] contains conflicting changes - conflict resolution may lose data!",
                   qPrintable(sourceEntry->title()),
                   qPrintable(sourceEntry->uuidToHex()));
        if (changes) {
            *changes << Change{
                Change::Type::Unspecified, *sourceEntry, []() {}, tr("Entry conflict - may cause data loss!")};
        }
    }

    if (targetModificationTime < sourceModificationTime) {
        if (preferLocal && merged.contains(targetModificationTime)) {
            // forcefully apply the local history item
            delete merged.take(targetModificationTime);
        }
        if (!merged.contains(targetModificationTime)) {
            merged[targetModificationTime] = targetEntry->clone(Entry::CloneNoFlags);
        }
    } else if (targetModificationTime > sourceModificationTime) {
        if (preferRemote && !merged.contains(sourceModificationTime)) {
            // forcefully apply the remote history item
            delete merged.take(sourceModificationTime);
        }
        if (!merged.contains(sourceModificationTime)) {
            merged[sourceModificationTime] = sourceEntry->clone(Entry::CloneNoFlags);
        }
    }

    bool changed = false;
    const auto updatedHistoryItems = merged.values();
    for (int i = 0; i < maxItems; ++i) {
        const Entry* oldEntry = targetHistoryItems.value(targetHistoryItems.count() - i);
        const Entry* newEntry = updatedHistoryItems.value(updatedHistoryItems.count() - i);
        if (!oldEntry && !newEntry) {
            continue;
        }
        if (oldEntry && newEntry && oldEntry->equals(newEntry, CompareItemIgnoreMilliseconds)) {
            continue;
        }
        changed = true;
        break;
    }
    if (!changed) {
        qDeleteAll(updatedHistoryItems);
        return false;
    }
    // We need to prevent any modification to the database since every change should be tracked either
    // in a clone history item or in the Entry itself
    const TimeInfo timeInfo = targetEntry->timeInfo();
    const bool blockedSignals = targetEntry->blockSignals(true);
    bool updateTimeInfo = targetEntry->canUpdateTimeinfo();
    targetEntry->setUpdateTimeinfo(false);
    targetEntry->removeHistoryItems(targetHistoryItems);
    for (Entry* historyItem : merged) {
        Q_ASSERT(!historyItem->parent());
        targetEntry->addHistoryItem(historyItem);
    }
    targetEntry->truncateHistory();
    targetEntry->blockSignals(blockedSignals);
    targetEntry->setUpdateTimeinfo(updateTimeInfo);
    Q_ASSERT(timeInfo == targetEntry->timeInfo());
    Q_UNUSED(timeInfo);
    return true;
}

QList<Merger::PreprocessedDeletion> Merger::preprocessDeletions(const MergeContext& context)
{
    QMap<QUuid, PreprocessedDeletion> mergedDeletions;

    const auto targetDeletions = context.m_targetDb->deletedObjects();
    const auto sourceDeletions = context.m_sourceDb->deletedObjects();

    for (const auto& deletion : (targetDeletions + sourceDeletions)) {
        if (!mergedDeletions.contains(deletion.uuid)) {
            auto* targetEntry = context.m_targetRootGroup->findEntryByUuid(deletion.uuid);
            auto* targetGroup = context.m_targetRootGroup->findGroupByUuid(deletion.uuid);
            const auto* sourceEntry = context.m_sourceRootGroup->findEntryByUuid(deletion.uuid);
            const auto* sourceGroup = context.m_sourceRootGroup->findGroupByUuid(deletion.uuid);

            mergedDeletions[deletion.uuid] =
                PreprocessedDeletion{deletion, targetEntry, sourceEntry, targetGroup, sourceGroup};
        } else if (mergedDeletions[deletion.uuid].deletion.deletionTime < deletion.deletionTime) {
            // only keep newest deletion
            // TODO: discuss: I inverted old logic; this simplifies check behavior for group deletion;
            // is this a problem somewhere else?
            mergedDeletions[deletion.uuid].deletion = deletion;
        }
    }

    return mergedDeletions.values();
}

void Merger::processEntryDeletion(const PreprocessedDeletion& object,
                                  QList<DeletedObject>& deletions,
                                  ChangeList& changes)
{
    if (isNewer(object.targetEntry, object.deletion.deletionTime)
        || isNewer(object.sourceEntry, object.deletion.deletionTime)) {
        // keep deleted entry since it was changed after deletion date
        return;
    }
    deletions << object.deletion;

    if (object.targetEntry) {
        changes << Change{Change::Type::Deleted,
                          *object.targetEntry,
                          [this, entry = object.targetEntry]() {
                              // Entry is inserted into deletedObjects after deletions are processed
                              eraseEntry(entry);
                          },
                          object.targetEntry->group() ? tr("Deleting child") : tr("Deleting orphan")};
    }
}

void Merger::processGroupDeletion(const PreprocessedDeletion& object,
                                  QList<PreprocessedDeletion>& unprocessedDeletions,
                                  QList<DeletedObject>& deletions,
                                  ChangeList& changes)
{
    auto allChildrenProcessed = [&unprocessedDeletions](const Group* group) {
        if (!group || !group->hasChildren()) {
            return true;
        }
        QSet<Group*> unprocessedGroups;
        unprocessedGroups.reserve(unprocessedDeletions.size());
        for (auto& unprocessedDeletion : unprocessedDeletions) {
            unprocessedGroups << unprocessedDeletion.targetGroup;
        }
        return unprocessedGroups.intersects(group->children().toSet());
    };

    if (!allChildrenProcessed(object.targetGroup)) { // TODO: see if sourceGroups has to be used here as well
        // we need to finish all children before we are able to determine if the group can be removed
        unprocessedDeletions << object;
        return;
    }
    if (isNewer(object.targetGroup, object.deletion.deletionTime)
        || isNewer(object.sourceGroup, object.deletion.deletionTime)) {
        // keep deleted group since it was changed after deletion date
        return;
    }
    if (hasAnyEntryOrChildModifiedAfterTime(object.targetGroup, object.deletion.deletionTime)
        || hasAnyEntryOrChildModifiedAfterTime(object.sourceGroup, object.deletion.deletionTime)) {
        // keep deleted group since it contains undeleted content
        return; // TODO: discuss if add change because deletion is ignored
    }
    deletions << object.deletion;

    if (object.targetGroup) {
        changes << Change{Change::Type::Deleted,
                          *object.targetGroup,
                          [this, group = object.targetGroup]() { eraseGroup(group); },
                          object.targetGroup->parentGroup() ? tr("Deleting child") : tr("Deleting orphan")};
    }
}

Merger::ChangeList Merger::mergeDeletions(const MergeContext& context)
{
    ChangeList changes;
    Group::MergeMode mergeMode = m_mode == Group::Default ? context.m_targetGroup->mergeMode() : m_mode;
    if (mergeMode != Group::Synchronize) {
        // no deletions are applied for any other strategy!
        return changes;
    }

    auto unprocessedDeletions = preprocessDeletions(context);
    QList<DeletedObject> deletions;
    ChangeList entryChanges;
    ChangeList groupChanges;
    while (!unprocessedDeletions.empty()) {
        auto object = unprocessedDeletions.takeFirst();
        if (object.targetEntry || object.sourceEntry) {
            processEntryDeletion(object, deletions, entryChanges);
        } else if (object.targetGroup || object.sourceGroup) {
            processGroupDeletion(object, unprocessedDeletions, deletions, groupChanges);
        } else {
            // entry/group was already deleted in both source and target database,
            // thus we can simply add it to the deleted objects
            deletions << object.deletion;
        }
    }
    // entries have to be merged/deleted first before groups can be deleted
    changes << entryChanges << groupChanges;

    // Put every deletion to the earliest date of deletion
    if (deletions != context.m_targetDb->deletedObjects()) {
        changes << Change{[context, deletions]() { context.m_targetDb->setDeletedObjects(deletions); },
                          tr("Updated deleted objects")};
    }
    return changes;
}

Merger::ChangeList Merger::mergeMetadata(const MergeContext& context)
{
    // TODO HNH: missing handling of recycle bin, names, templates for groups and entries,
    //           public data (entries of newer dict override keys of older dict - ignoring
    //           their own age - it is enough if one entry of the whole dict is newer) => possible lost update
    ChangeList changes;
    auto* sourceMetadata = context.m_sourceDb->metadata();
    auto* targetMetadata = context.m_targetDb->metadata();

    for (const auto& iconUuid : sourceMetadata->customIconsOrder()) {
        if (!targetMetadata->hasCustomIcon(iconUuid)) {
            changes << Change{[sourceMetadata, targetMetadata, iconUuid]() {
                                  targetMetadata->addCustomIcon(iconUuid, sourceMetadata->customIcon(iconUuid));
                              },
                              tr("Adding missing icon %1").arg(QString::fromLatin1(iconUuid.toRfc4122().toHex()))};
        }
    }

    // Merge Custom Data if source is newer
    const auto targetCustomDataModificationTime = targetMetadata->customData()->lastModified();
    const auto sourceCustomDataModificationTime = sourceMetadata->customData()->lastModified();
    if (!targetMetadata->customData()->contains(CustomData::LastModified)
        || (targetCustomDataModificationTime.isValid() && sourceCustomDataModificationTime.isValid()
            && targetCustomDataModificationTime < sourceCustomDataModificationTime)) {
        const auto sourceCustomDataKeys = sourceMetadata->customData()->keys();
        const auto targetCustomDataKeys = targetMetadata->customData()->keys();

        // Check missing keys from source. Remove those from target
        for (const auto& key : targetCustomDataKeys) {
            // Do not remove protected custom data
            if (!sourceMetadata->customData()->contains(key) && !sourceMetadata->customData()->isProtected(key)) {
                auto value = targetMetadata->customData()->value(key);
                changes << Change{[targetMetadata, key]() { targetMetadata->customData()->remove(key); },
                                  tr("Removed custom data %1 [%2]").arg(key, value)};
            }
        }

        // Transfer new/existing keys
        for (const auto& key : sourceCustomDataKeys) {
            // Don't merge this meta field, it is updated automatically.
            if (key == CustomData::LastModified) {
                continue;
            }

            auto sourceValue = sourceMetadata->customData()->value(key);
            auto targetValue = targetMetadata->customData()->value(key);
            // Merge only if the values are not the same.
            if (sourceValue != targetValue) {
                ;
                changes << Change{
                    [targetMetadata, key, sourceValue]() { targetMetadata->customData()->set(key, sourceValue); },
                    tr("Adding custom data %1 [%2]").arg(key, sourceValue)};
            }
        }
    }

    return changes;
}
