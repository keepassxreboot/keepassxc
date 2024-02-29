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
    QStringList merge();

private:
    typedef QString Change;
    typedef QStringList ChangeList;

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

private:
    MergeContext m_context;
    Group::MergeMode m_mode;
};

#endif // KEEPASSXC_MERGER_H
