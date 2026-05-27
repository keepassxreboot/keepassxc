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
    Merger(const Database* sourceDb, Database* targetDb);
    Merger(const Group* sourceGroup, Group* targetGroup);
    void setForcedMergeMode(Group::MergeMode mode);
    void resetForcedMergeMode();
    void setSkipDatabaseCustomData(bool state);

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
            Metadata,
        };

        Change(Type type, QString details);
        Change(Type type, const Group& group, QString details = "");
        Change(Type type, const Entry& entry, QString details = "");
        explicit Change(QString details = "");

        [[nodiscard]] Type type() const;
        [[nodiscard]] QString typeString() const;
        [[nodiscard]] const QString& title() const;
        [[nodiscard]] const QString& group() const;
        [[nodiscard]] const QUuid& uuid() const;
        [[nodiscard]] const QString& details() const;

        [[nodiscard]] QString toString() const;
        void merge();

        bool operator==(const Change& other) const;
        bool operator!=(const Change& other) const;

    private:
        Type m_type{Type::Unspecified};
        QString m_title;
        QString m_group;
        QUuid m_uuid;
        QString m_details;
    };

    using ChangeList = QList<Change>;

    ChangeList merge(bool dryRun = false);

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

    ChangeList mergeGroup(const MergeContext& context);
    ChangeList mergeDeletions(const MergeContext& context);
    ChangeList mergeMetadata(const MergeContext& context);
    bool mergeHistory(const Entry* sourceEntry, Entry* targetEntry, Group::MergeMode mergeMethod, const int maxItems);
    void moveEntry(Entry* entry, Group* targetGroup);
    void moveGroup(Group* group, Group* targetGroup);
    // remove an entry without a trace in the deletedObjects - needed for elimination of cloned entries
    void eraseEntry(Entry* entry);
    // remove an entry without a trace in the deletedObjects - needed for elimination of cloned entries
    void eraseGroup(Group* group);
    ChangeList resolveEntryConflict(const MergeContext& context, const Entry* existingEntry, Entry* otherEntry);
    ChangeList resolveGroupConflict(const MergeContext& context, const Group* existingGroup, Group* otherGroup);
    Merger::ChangeList resolveEntryConflict_MergeHistories(const MergeContext& context,
                                                           const Entry* sourceEntry,
                                                           Entry* targetEntry,
                                                           Group::MergeMode mergeMethod);

private:
    MergeContext m_context;
    Group::MergeMode m_mode;
    bool m_skipCustomData = false;
    bool m_dryRun = false;
};

#endif // KEEPASSXC_MERGER_H
