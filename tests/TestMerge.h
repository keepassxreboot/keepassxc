/*
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

#ifndef KEEPASSX_TESTMERGE_H
#define KEEPASSX_TESTMERGE_H

#include "core/Database.h"
#include <functional>

class TestMerge : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testMergeIntoNew();
    void testMergeNoChanges();
    void testResolveConflictNewer();
    void testResolveConflictExisting();
    void testResolveGroupConflictOlder();
    void testMergeNotModified();
    void testMergeModified();
    void testResolveConflictEntry_Synchronize();
    void testResolveConflictEntry_KeepNewer();
    void testDeletionConflictEntry_Synchronized();
    void testDeletionConflictEntry_KeepNewer();
    void testMoveEntry();
    void testMoveEntryPreserveChanges();
    void testMoveEntryIntoNewGroup();
    void testCreateNewGroups();
    void testUpdateEntryDifferentLocation();
    void testUpdateGroup();
    void testUpdateGroupLocation();
    void testMergeAndSync();
    void testMergeCustomIcons();
    void testMergeDuplicateCustomIcons();
    void testMetadata();
    void testCustomData();
    void testDeletedEntry();
    void testDeletedGroup();
    void testDeletedRevertedEntry();
    void testDeletedRevertedGroup();

private:
    Database* createTestDatabase();
    Database* createTestDatabaseStructureClone(Database* source, int entryFlags, int groupFlags);
    void testResolveConflictTemplate(int mergeMode,
                                     std::function<void(Database*, const QMap<const char*, QDateTime>&)> verification);
    void testDeletionConflictTemplate(int mergeMode,
                                      std::function<void(Database*, const QMap<QString, QUuid>&)> verification);
    static void assertDeletionNewerOnly(Database* db, const QMap<QString, QUuid>& identifiers);
    static void assertDeletionLocalOnly(Database* db, const QMap<QString, QUuid>& identifiers);
    static void assertUpdateMergedEntry1(Entry* entry, const QMap<const char*, QDateTime>& timestamps);
    static void assertUpdateReappliedEntry2(Entry* entry, const QMap<const char*, QDateTime>& timestamps);
    static void assertUpdateReappliedEntry1(Entry* entry, const QMap<const char*, QDateTime>& timestamps);
    static void assertUpdateMergedEntry2(Entry* entry, const QMap<const char*, QDateTime>& timestamps);
};

#endif // KEEPASSX_TESTMERGE_H
