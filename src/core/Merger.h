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

#ifndef KEEPASSXC_MERGER_H
#define KEEPASSXC_MERGER_H

#include "core/Group.h"

class Database;
class Entry;

class Merger : public QObject
{
    Q_OBJECT
public:
    class Change
    {
    public:
        enum class Type
        {
            Unspecified,
            Added,
            Modified,
            Moved,
            Deleted,
        };

        using MergeOperation = std::function<void()>;

        Change(Type type, const Group& group, MergeOperation mergeOperation, QString details = "");
        Change(Type type, const Entry& entry, MergeOperation mergeOperation, QString details = "");
        explicit Change(MergeOperation mergeOperation, QString details = "");

        [[nodiscard]] Type type() const;
        [[nodiscard]] QString typeString() const;
        [[nodiscard]] const QString& title() const;
        [[nodiscard]] const QString& group() const;
        [[nodiscard]] const QUuid& uuid() const;
        [[nodiscard]] const QString& details() const;

        [[nodiscard]] QString toString() const;
        void merge();

    private:
        Type m_type{Type::Unspecified};
        QString m_title;
        QString m_group;
        QUuid m_uuid;
        QString m_details;
        MergeOperation m_mergeOperation;
    };
    using ChangeList = QList<Change>;

    Merger(const Database* sourceDb, Database* targetDb);
    Merger(const Group* sourceGroup, Group* targetGroup);
    void setForcedMergeMode(Group::MergeMode mode);
    void resetForcedMergeMode();
    const ChangeList& changes();
    const ChangeList& merge(bool recalculateChanges = true);

private:
    struct MergeContext
    {
        QPointer<const Database> m_sourceDb;
        QPointer<Database> m_targetDb;
        QPointer<const Group> m_sourceRootGroup;
        QPointer<Group> m_targetRootGroup;
        QPointer<const Group> m_sourceGroup;
        QPointer<Group> m_targetGroup;
    };
    [[nodiscard]] Group::MergeMode mergeMode(const MergeContext& context) const;
    ChangeList mergeGroup(const MergeContext& context);
    ChangeList mergeDeletions(const MergeContext& context);
    ChangeList mergeMetadata(const MergeContext& context);
    bool mergeHistory(const Entry* sourceEntry,
                      Entry* targetEntry,
                      Group::MergeMode mergeMethod,
                      const int maxItems,
                      ChangeList* changes = nullptr);
    void moveEntry(Entry* entry, Group* targetGroup);
    void moveGroup(Group* group, Group* targetGroup);
    // remove an entry without a trace in the deletedObjects - needed for elemination cloned entries
    void eraseEntry(Entry* entry);
    // remove an entry without a trace in the deletedObjects - needed for elemination cloned entries
    void eraseGroup(Group* group);
    ChangeList resolveEntryConflict(const MergeContext& context, const Entry* existingEntry, Entry* otherEntry);
    ChangeList resolveGroupConflict(const MergeContext& context, const Group* existingGroup, Group* otherGroup);
    Merger::ChangeList resolveEntryConflict_MergeHistories(const MergeContext& context,
                                                           const Entry* sourceEntry,
                                                           Entry* targetEntry,
                                                           Group::MergeMode mergeMethod);
    struct PreprocessedDeletion
    {
        DeletedObject deletion;
        Entry* targetEntry = nullptr;
        const Entry* sourceEntry = nullptr;
        Group* targetGroup = nullptr;
        const Group* sourceGroup = nullptr;
    };
    static QList<PreprocessedDeletion> preprocessDeletions(const MergeContext& context);
    void processEntryDeletion(const PreprocessedDeletion& object, QList<DeletedObject>& deletions, ChangeList& changes);
    void processGroupDeletion(const PreprocessedDeletion& object,
                              QList<PreprocessedDeletion>& unprocessedDeletions,
                              QList<DeletedObject>& deletions,
                              ChangeList& changes);

private:
    MergeContext m_context;
    Group::MergeMode m_mode;
    std::unique_ptr<ChangeList> m_changes;
};

#endif // KEEPASSXC_MERGER_H
