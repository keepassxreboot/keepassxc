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

#include "TestMerge.h"
#include "TestGlobal.h"

#include "core/Metadata.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestMerge)

void TestMerge::initTestCase()
{
    qRegisterMetaType<Entry*>("Entry*");
    qRegisterMetaType<Group*>("Group*");
    QVERIFY(Crypto::init());
}

/**
 * Merge an existing database into a new one.
 * All the entries of the existing should end
 * up in the new one.
 */
void TestMerge::testMergeIntoNew()
{
    Database* dbSource = createTestDatabase();
    Database* dbDestination = new Database();

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->children().size(), 2);
    QCOMPARE(dbDestination->rootGroup()->children().at(0)->entries().size(), 2);
    // Test for retention of history
    QCOMPARE(dbDestination->rootGroup()->children().at(0)->entries().at(0)->historyItems().isEmpty(), false);

    delete dbDestination;
    delete dbSource;
}

/**
 * Merging when no changes occured should not
 * have any side effect.
 */
void TestMerge::testMergeNoChanges()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);
    QCOMPARE(dbSource->rootGroup()->entriesRecursive().size(), 2);

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);
    QCOMPARE(dbSource->rootGroup()->entriesRecursive().size(), 2);

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);
    QCOMPARE(dbSource->rootGroup()->entriesRecursive().size(), 2);

    delete dbDestination;
    delete dbSource;
}

/**
 * If the entry is updated in the source database, the update
 * should propagate in the destination database.
 */
void TestMerge::testResolveConflictNewer()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    // sanity check
    Group* group1 = dbSource->rootGroup()->findChildByName("group1");
    QVERIFY(group1 != nullptr);
    QCOMPARE(group1->entries().size(), 2);

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);
    // make this entry newer than in destination db
    entry1->beginUpdate();
    entry1->setPassword("password");
    entry1->endUpdate();

    dbDestination->merge(dbSource);

    // sanity check
    group1 = dbDestination->rootGroup()->findChildByName("group1");
    QVERIFY(group1 != nullptr);
    QCOMPARE(group1->entries().size(), 2);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QVERIFY(entry1->group() != nullptr);
    QCOMPARE(entry1->password(), QString("password"));

    // When updating an entry, it should not end up in the
    // deleted objects.
    for (DeletedObject deletedObject : dbDestination->deletedObjects()) {
        QVERIFY(deletedObject.uuid != entry1->uuid());
    }

    delete dbDestination;
    delete dbSource;
}

/**
 * If the entry is updated in the source database, and the
 * destination database after, the entry should remain the
 * same.
 */
void TestMerge::testResolveConflictOlder()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    // sanity check
    Group* group1 = dbSource->rootGroup()->findChildByName("group1");
    QVERIFY(group1 != nullptr);
    QCOMPARE(group1->entries().size(), 2);

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);
    // make this entry newer than in destination db
    entry1->beginUpdate();
    entry1->setPassword("password1");
    entry1->endUpdate();

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);
    // make this entry newer than in destination db
    entry1->beginUpdate();
    entry1->setPassword("password2");
    entry1->endUpdate();

    dbDestination->merge(dbSource);

    // sanity check
    group1 = dbDestination->rootGroup()->findChildByName("group1");
    QVERIFY(group1 != nullptr);
    QCOMPARE(group1->entries().size(), 2);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QCOMPARE(entry1->password(), QString("password2"));

    // When updating an entry, it should not end up in the
    // deleted objects.
    for (DeletedObject deletedObject : dbDestination->deletedObjects()) {
        QVERIFY(deletedObject.uuid != entry1->uuid());
    }

    delete dbDestination;
    delete dbSource;
}

/**
 * Tests the KeepBoth merge mode.
 */
void TestMerge::testResolveConflictKeepBoth()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneIncludeHistory, Group::CloneIncludeEntries));

    // sanity check
    QCOMPARE(dbDestination->rootGroup()->children().at(0)->entries().size(), 2);

    // make this entry newer than in original db
    Entry* updatedEntry = dbDestination->rootGroup()->children().at(0)->entries().at(0);
    TimeInfo updatedTimeInfo = updatedEntry->timeInfo();
    updatedTimeInfo.setLastModificationTime(updatedTimeInfo.lastModificationTime().addYears(1));
    updatedEntry->setTimeInfo(updatedTimeInfo);

    dbDestination->rootGroup()->setMergeMode(Group::MergeMode::KeepBoth);

    dbDestination->merge(dbSource);

    // one entry is duplicated because of mode
    QCOMPARE(dbDestination->rootGroup()->children().at(0)->entries().size(), 3);
    QCOMPARE(dbDestination->rootGroup()->children().at(0)->entries().at(0)->historyItems().isEmpty(), false);
    // the older entry was merged from the other db as last in the group
    Entry* olderEntry = dbDestination->rootGroup()->children().at(0)->entries().at(2);
    QVERIFY2(olderEntry->attributes()->hasKey("merged"), "older entry is marked with an attribute \"merged\"");
    QCOMPARE(olderEntry->historyItems().isEmpty(), false);

    QVERIFY2(olderEntry->uuid() != updatedEntry->uuid(), "KeepBoth should not reuse the UUIDs when cloning.");

    delete dbSource;
    delete dbDestination;
}

/**
 * The location of an entry should be updated in the
 * destination database.
 */
void TestMerge::testMoveEntry()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    Group* group2 = dbSource->rootGroup()->findChildByName("group2");
    QVERIFY(group2 != nullptr);

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);
    entry1->setGroup(group2);
    QCOMPARE(entry1->group()->name(), QString("group2"));

    dbDestination->merge(dbSource);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QCOMPARE(entry1->group()->name(), QString("group2"));
    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    delete dbDestination;
    delete dbSource;
}

/**
 * The location of an entry should be updated in the
 * destination database, but changes from the destination
 * database should be preserved.
 */
void TestMerge::testMoveEntryPreserveChanges()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    Group* group2 = dbSource->rootGroup()->findChildByName("group2");
    QVERIFY(group2 != nullptr);

    QTest::qSleep(1);
    entry1->setGroup(group2);
    QCOMPARE(entry1->group()->name(), QString("group2"));

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);

    QTest::qSleep(1);
    entry1->beginUpdate();
    entry1->setPassword("password");
    entry1->endUpdate();

    dbDestination->merge(dbSource);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QCOMPARE(entry1->group()->name(), QString("group2"));
    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);
    QCOMPARE(entry1->password(), QString("password"));

    delete dbDestination;
    delete dbSource;
}

void TestMerge::testCreateNewGroups()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    QTest::qSleep(1);
    Group* group3 = new Group();
    group3->setName("group3");
    group3->setUuid(QUuid::createUuid());
    group3->setParent(dbSource->rootGroup());

    dbDestination->merge(dbSource);

    group3 = dbDestination->rootGroup()->findChildByName("group3");
    QVERIFY(group3 != nullptr);
    QCOMPARE(group3->name(), QString("group3"));

    delete dbDestination;
    delete dbSource;
}

void TestMerge::testMoveEntryIntoNewGroup()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    QTest::qSleep(1);
    Group* group3 = new Group();
    group3->setName("group3");
    group3->setUuid(QUuid::createUuid());
    group3->setParent(dbSource->rootGroup());

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    entry1->setGroup(group3);

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    group3 = dbDestination->rootGroup()->findChildByName("group3");
    QVERIFY(group3 != nullptr);
    QCOMPARE(group3->name(), QString("group3"));
    QCOMPARE(group3->entries().size(), 1);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QCOMPARE(entry1->group()->name(), QString("group3"));

    delete dbDestination;
    delete dbSource;
}

/**
 * Even though the entries' locations are no longer
 * the same, we will keep associating them.
 */
void TestMerge::testUpdateEntryDifferentLocation()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    Group* group3 = new Group();
    group3->setName("group3");
    group3->setUuid(QUuid::createUuid());
    group3->setParent(dbDestination->rootGroup());

    Entry* entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    entry1->setGroup(group3);
    QUuid uuidBeforeSyncing = entry1->uuid();

    // Change the entry in the source db.
    QTest::qSleep(1);
    entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    entry1->beginUpdate();
    entry1->setUsername("username");
    entry1->endUpdate();

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    entry1 = dbDestination->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    QVERIFY(entry1->group() != nullptr);
    QCOMPARE(entry1->username(), QString("username"));
    QCOMPARE(entry1->group()->name(), QString("group3"));
    QCOMPARE(uuidBeforeSyncing, entry1->uuid());

    delete dbDestination;
    delete dbSource;
}

/**
 * Groups should be updated using the uuids.
 */
void TestMerge::testUpdateGroup()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    QTest::qSleep(1);

    Group* group2 = dbSource->rootGroup()->findChildByName("group2");
    group2->setName("group2 renamed");
    group2->setNotes("updated notes");
    QUuid customIconId = QUuid::createUuid();
    QImage customIcon;
    dbSource->metadata()->addCustomIcon(customIconId, customIcon);
    group2->setIcon(customIconId);

    Entry* entry1 = dbSource->rootGroup()->findEntry("entry1");
    QVERIFY(entry1 != nullptr);
    entry1->setGroup(group2);
    entry1->setTitle("entry1 renamed");
    QUuid uuidBeforeSyncing = entry1->uuid();

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    entry1 = dbDestination->rootGroup()->findEntry("entry1 renamed");
    QVERIFY(entry1 != nullptr);
    QVERIFY(entry1->group() != nullptr);
    QCOMPARE(entry1->group()->name(), QString("group2 renamed"));
    QCOMPARE(uuidBeforeSyncing, entry1->uuid());

    group2 = dbDestination->rootGroup()->findChildByName("group2 renamed");
    QCOMPARE(group2->notes(), QString("updated notes"));
    QCOMPARE(group2->iconUuid(), customIconId);

    delete dbDestination;
    delete dbSource;
}

void TestMerge::testUpdateGroupLocation()
{
    Database* dbDestination = createTestDatabase();
    Group* group3 = new Group();
    QUuid group3Uuid = QUuid::createUuid();
    group3->setUuid(group3Uuid);
    group3->setName("group3");
    group3->setParent(dbDestination->rootGroup()->findChildByName("group1"));

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    // Sanity check
    group3 = dbSource->rootGroup()->findChildByUuid(group3Uuid);
    QVERIFY(group3 != nullptr);

    QTest::qSleep(1);

    group3->setParent(dbSource->rootGroup()->findChildByName("group2"));

    dbDestination->merge(dbSource);
    group3 = dbDestination->rootGroup()->findChildByUuid(group3Uuid);
    QVERIFY(group3 != nullptr);
    QCOMPARE(group3->parent(), dbDestination->rootGroup()->findChildByName("group2"));

    dbDestination->merge(dbSource);
    group3 = dbDestination->rootGroup()->findChildByUuid(group3Uuid);
    QVERIFY(group3 != nullptr);
    QCOMPARE(group3->parent(), dbDestination->rootGroup()->findChildByName("group2"));

    delete dbDestination;
    delete dbSource;
}

/**
 * The first merge should create new entries, the
 * second should only sync them, since they have
 * been created with the same UUIDs.
 */
void TestMerge::testMergeAndSync()
{
    Database* dbDestination = new Database();
    Database* dbSource = createTestDatabase();

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 0);

    dbDestination->merge(dbSource);

    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    dbDestination->merge(dbSource);

    // Still only 2 entries, since now we detect which are already present.
    QCOMPARE(dbDestination->rootGroup()->entriesRecursive().size(), 2);

    delete dbDestination;
    delete dbSource;
}

/**
 * Custom icons should be brought over when merging.
 */
void TestMerge::testMergeCustomIcons()
{
    Database* dbDestination = new Database();
    Database* dbSource = createTestDatabase();

    QUuid customIconId = QUuid::createUuid();
    QImage customIcon;

    dbSource->metadata()->addCustomIcon(customIconId, customIcon);
    // Sanity check.
    QVERIFY(dbSource->metadata()->containsCustomIcon(customIconId));

    dbDestination->merge(dbSource);

    QVERIFY(dbDestination->metadata()->containsCustomIcon(customIconId));

    delete dbDestination;
    delete dbSource;
}

/**
 * If the group is updated in the source database, and the
 * destination database after, the group should remain the
 * same.
 */
void TestMerge::testResolveGroupConflictOlder()
{
    Database* dbDestination = createTestDatabase();

    Database* dbSource = new Database();
    dbSource->setRootGroup(dbDestination->rootGroup()->clone(Entry::CloneNoFlags, Group::CloneIncludeEntries));

    // sanity check
    Group* group1 = dbSource->rootGroup()->findChildByName("group1");
    QVERIFY(group1 != nullptr);

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);
    group1->setName("group1 updated in source");

    // Make sure the two changes have a different timestamp.
    QTest::qSleep(1);

    group1 = dbDestination->rootGroup()->findChildByName("group1");
    group1->setName("group1 updated in destination");

    dbDestination->merge(dbSource);

    // sanity check
    group1 = dbDestination->rootGroup()->findChildByName("group1 updated in destination");
    QVERIFY(group1 != nullptr);

    delete dbDestination;
    delete dbSource;
}

Database* TestMerge::createTestDatabase()
{
    Database* db = new Database();

    Group* group1 = new Group();
    group1->setName("group1");
    group1->setUuid(QUuid::createUuid());

    Group* group2 = new Group();
    group2->setName("group2");
    group2->setUuid(QUuid::createUuid());

    Entry* entry1 = new Entry();
    Entry* entry2 = new Entry();

    // Give Entry 1 a history
    entry1->beginUpdate();
    entry1->setGroup(group1);
    entry1->setUuid(QUuid::createUuid());
    entry1->setTitle("entry1");
    entry1->endUpdate();

    // Give Entry 2 a history
    entry2->beginUpdate();
    entry2->setGroup(group1);
    entry2->setUuid(QUuid::createUuid());
    entry2->setTitle("entry2");
    entry2->endUpdate();

    group1->setParent(db->rootGroup());
    group2->setParent(db->rootGroup());

    return db;
}
