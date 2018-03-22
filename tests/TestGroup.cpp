/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestGroup.h"
#include "TestGlobal.h"

#include <QSignalSpy>

#include "core/Metadata.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestGroup)

void TestGroup::initTestCase()
{
    qRegisterMetaType<Entry*>("Entry*");
    qRegisterMetaType<Group*>("Group*");
    QVERIFY(Crypto::init());
}

void TestGroup::testParenting()
{
    Database* db = new Database();
    QPointer<Group> rootGroup = db->rootGroup();
    Group* tmpRoot = new Group();

    QPointer<Group> g1 = new Group();
    QPointer<Group> g2 = new Group();
    QPointer<Group> g3 = new Group();
    QPointer<Group> g4 = new Group();

    g1->setParent(tmpRoot);
    g2->setParent(tmpRoot);
    g3->setParent(tmpRoot);
    g4->setParent(tmpRoot);

    g2->setParent(g1);
    g4->setParent(g3);
    g3->setParent(g1);
    g1->setParent(db->rootGroup());

    QVERIFY(g1->parent() == rootGroup);
    QVERIFY(g2->parent() == g1);
    QVERIFY(g3->parent() == g1);
    QVERIFY(g4->parent() == g3);

    QVERIFY(g1->database() == db);
    QVERIFY(g2->database() == db);
    QVERIFY(g3->database() == db);
    QVERIFY(g4->database() == db);

    QCOMPARE(tmpRoot->children().size(), 0);
    QCOMPARE(rootGroup->children().size(), 1);
    QCOMPARE(g1->children().size(), 2);
    QCOMPARE(g2->children().size(), 0);
    QCOMPARE(g3->children().size(), 1);
    QCOMPARE(g4->children().size(), 0);

    QVERIFY(rootGroup->children().at(0) == g1);
    QVERIFY(rootGroup->children().at(0) == g1);
    QVERIFY(g1->children().at(0) == g2);
    QVERIFY(g1->children().at(1) == g3);
    QVERIFY(g3->children().contains(g4));

    Group* g5 = new Group();
    Group* g6 = new Group();
    g5->setParent(db->rootGroup());
    g6->setParent(db->rootGroup());
    QVERIFY(db->rootGroup()->children().at(1) == g5);
    QVERIFY(db->rootGroup()->children().at(2) == g6);

    g5->setParent(db->rootGroup());
    QVERIFY(db->rootGroup()->children().at(1) == g6);
    QVERIFY(db->rootGroup()->children().at(2) == g5);

    QSignalSpy spy(db, SIGNAL(groupDataChanged(Group*)));
    g2->setName("test");
    g4->setName("test");
    g3->setName("test");
    g1->setName("test");
    g3->setIcon(QUuid::createUuid());
    g1->setIcon(2);
    QCOMPARE(spy.count(), 6);
    delete db;

    QVERIFY(rootGroup.isNull());
    QVERIFY(g1.isNull());
    QVERIFY(g2.isNull());
    QVERIFY(g3.isNull());
    QVERIFY(g4.isNull());
    delete tmpRoot;
}

void TestGroup::testSignals()
{
    Database* db = new Database();
    Database* db2 = new Database();
    QPointer<Group> root = db->rootGroup();

    QSignalSpy spyAboutToAdd(db, SIGNAL(groupAboutToAdd(Group*, int)));
    QSignalSpy spyAdded(db, SIGNAL(groupAdded()));
    QSignalSpy spyAboutToRemove(db, SIGNAL(groupAboutToRemove(Group*)));
    QSignalSpy spyRemoved(db, SIGNAL(groupRemoved()));
    QSignalSpy spyAboutToMove(db, SIGNAL(groupAboutToMove(Group*, Group*, int)));
    QSignalSpy spyMoved(db, SIGNAL(groupMoved()));

    QSignalSpy spyAboutToAdd2(db2, SIGNAL(groupAboutToAdd(Group*, int)));
    QSignalSpy spyAdded2(db2, SIGNAL(groupAdded()));
    QSignalSpy spyAboutToRemove2(db2, SIGNAL(groupAboutToRemove(Group*)));
    QSignalSpy spyRemoved2(db2, SIGNAL(groupRemoved()));
    QSignalSpy spyAboutToMove2(db2, SIGNAL(groupAboutToMove(Group*, Group*, int)));
    QSignalSpy spyMoved2(db2, SIGNAL(groupMoved()));

    Group* g1 = new Group();
    Group* g2 = new Group();

    g1->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyAboutToMove.count(), 0);
    QCOMPARE(spyMoved.count(), 0);

    g2->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyAboutToMove.count(), 0);
    QCOMPARE(spyMoved.count(), 0);

    g2->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyAboutToMove.count(), 0);
    QCOMPARE(spyMoved.count(), 0);

    g2->setParent(root, 0);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyAboutToMove.count(), 1);
    QCOMPARE(spyMoved.count(), 1);

    g1->setParent(g2);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyAboutToMove.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    delete g1;
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);
    QCOMPARE(spyAboutToMove.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    g2->setParent(db2->rootGroup());
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 2);
    QCOMPARE(spyRemoved.count(), 2);
    QCOMPARE(spyAboutToMove.count(), 2);
    QCOMPARE(spyMoved.count(), 2);
    QCOMPARE(spyAboutToAdd2.count(), 1);
    QCOMPARE(spyAdded2.count(), 1);
    QCOMPARE(spyAboutToRemove2.count(), 0);
    QCOMPARE(spyRemoved2.count(), 0);
    QCOMPARE(spyAboutToMove2.count(), 0);
    QCOMPARE(spyMoved2.count(), 0);

    Group* g3 = new Group();
    Group* g4 = new Group();

    g3->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 3);
    QCOMPARE(spyAdded.count(), 3);
    QCOMPARE(spyAboutToRemove.count(), 2);
    QCOMPARE(spyRemoved.count(), 2);
    QCOMPARE(spyAboutToMove.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    g4->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 4);
    QCOMPARE(spyAdded.count(), 4);
    QCOMPARE(spyAboutToRemove.count(), 2);
    QCOMPARE(spyRemoved.count(), 2);
    QCOMPARE(spyAboutToMove.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    g3->setParent(root);
    QCOMPARE(spyAboutToAdd.count(), 4);
    QCOMPARE(spyAdded.count(), 4);
    QCOMPARE(spyAboutToRemove.count(), 2);
    QCOMPARE(spyRemoved.count(), 2);
    QCOMPARE(spyAboutToMove.count(), 3);
    QCOMPARE(spyMoved.count(), 3);

    delete db;
    delete db2;

    QVERIFY(root.isNull());
}

void TestGroup::testEntries()
{
    Group* group = new Group();

    QPointer<Entry> entry1 = new Entry();
    entry1->setGroup(group);

    QPointer<Entry> entry2 = new Entry();
    entry2->setGroup(group);

    QCOMPARE(group->entries().size(), 2);
    QVERIFY(group->entries().at(0) == entry1);
    QVERIFY(group->entries().at(1) == entry2);

    delete group;

    QVERIFY(entry1.isNull());
    QVERIFY(entry2.isNull());
}

void TestGroup::testDeleteSignals()
{
    QScopedPointer<Database> db(new Database());
    Group* groupRoot = db->rootGroup();
    Group* groupChild = new Group();
    Group* groupChildChild = new Group();
    groupRoot->setObjectName("groupRoot");
    groupChild->setObjectName("groupChild");
    groupChildChild->setObjectName("groupChildChild");
    groupChild->setParent(groupRoot);
    groupChildChild->setParent(groupChild);
    QSignalSpy spyAboutToRemove(db.data(), SIGNAL(groupAboutToRemove(Group*)));
    QSignalSpy spyRemoved(db.data(), SIGNAL(groupRemoved()));

    delete groupChild;
    QVERIFY(groupRoot->children().isEmpty());
    QCOMPARE(spyAboutToRemove.count(), 2);
    QCOMPARE(spyRemoved.count(), 2);

    Group* group = new Group();
    Entry* entry = new Entry();
    entry->setGroup(group);
    QSignalSpy spyEntryAboutToRemove(group, SIGNAL(entryAboutToRemove(Entry*)));
    QSignalSpy spyEntryRemoved(group, SIGNAL(entryRemoved(Entry*)));

    delete entry;
    QVERIFY(group->entries().isEmpty());
    QCOMPARE(spyEntryAboutToRemove.count(), 1);
    QCOMPARE(spyEntryRemoved.count(), 1);
    delete group;

    QScopedPointer<Database> db2(new Database());
    Group* groupRoot2 = db2->rootGroup();
    Group* group2 = new Group();
    group2->setParent(groupRoot2);
    Entry* entry2 = new Entry();
    entry2->setGroup(group2);
    QSignalSpy spyEntryAboutToRemove2(group2, SIGNAL(entryAboutToRemove(Entry*)));
    QSignalSpy spyEntryRemoved2(group2, SIGNAL(entryRemoved(Entry*)));

    delete group2;
    QCOMPARE(spyEntryAboutToRemove2.count(), 1);
    QCOMPARE(spyEntryRemoved2.count(), 1);
}

void TestGroup::testCopyCustomIcon()
{
    QScopedPointer<Database> dbSource(new Database());

    QUuid groupIconUuid = QUuid::createUuid();
    QImage groupIcon(16, 16, QImage::Format_RGB32);
    groupIcon.setPixel(0, 0, qRgb(255, 0, 0));
    dbSource->metadata()->addCustomIcon(groupIconUuid, groupIcon);

    QUuid entryIconUuid = QUuid::createUuid();
    QImage entryIcon(16, 16, QImage::Format_RGB32);
    entryIcon.setPixel(0, 0, qRgb(255, 0, 0));
    dbSource->metadata()->addCustomIcon(entryIconUuid, entryIcon);

    Group* group = new Group();
    group->setParent(dbSource->rootGroup());
    group->setIcon(groupIconUuid);
    QCOMPARE(group->icon(), groupIcon);

    Entry* entry = new Entry();
    entry->setGroup(dbSource->rootGroup());
    entry->setIcon(entryIconUuid);
    QCOMPARE(entry->icon(), entryIcon);

    QScopedPointer<Database> dbTarget(new Database());

    group->setParent(dbTarget->rootGroup());
    QVERIFY(dbTarget->metadata()->containsCustomIcon(groupIconUuid));
    QCOMPARE(dbTarget->metadata()->customIcon(groupIconUuid), groupIcon);
    QCOMPARE(group->icon(), groupIcon);

    entry->setGroup(dbTarget->rootGroup());
    QVERIFY(dbTarget->metadata()->containsCustomIcon(entryIconUuid));
    QCOMPARE(dbTarget->metadata()->customIcon(entryIconUuid), entryIcon);
    QCOMPARE(entry->icon(), entryIcon);
}

void TestGroup::testClone()
{
    QScopedPointer<Database> db(new Database());

    QScopedPointer<Group> originalGroup(new Group());
    originalGroup->setParent(db->rootGroup());
    originalGroup->setName("Group");
    originalGroup->setIcon(42);

    QScopedPointer<Entry> originalGroupEntry(new Entry());
    originalGroupEntry->setGroup(originalGroup.data());
    originalGroupEntry->setTitle("GroupEntryOld");
    originalGroupEntry->setIcon(43);
    originalGroupEntry->beginUpdate();
    originalGroupEntry->setTitle("GroupEntry");
    originalGroupEntry->endUpdate();

    QScopedPointer<Group> subGroup(new Group());
    subGroup->setParent(originalGroup.data());
    subGroup->setName("SubGroup");

    QScopedPointer<Entry> subGroupEntry(new Entry());
    subGroupEntry->setGroup(subGroup.data());
    subGroupEntry->setTitle("SubGroupEntry");

    QScopedPointer<Group> clonedGroup(originalGroup->clone());
    QVERIFY(!clonedGroup->parentGroup());
    QVERIFY(!clonedGroup->database());
    QVERIFY(clonedGroup->uuid() != originalGroup->uuid());
    QCOMPARE(clonedGroup->name(), QString("Group"));
    QCOMPARE(clonedGroup->iconNumber(), 42);
    QCOMPARE(clonedGroup->children().size(), 1);
    QCOMPARE(clonedGroup->entries().size(), 1);

    Entry* clonedGroupEntry = clonedGroup->entries().at(0);
    QVERIFY(clonedGroupEntry->uuid() != originalGroupEntry->uuid());
    QCOMPARE(clonedGroupEntry->title(), QString("GroupEntry"));
    QCOMPARE(clonedGroupEntry->iconNumber(), 43);
    QCOMPARE(clonedGroupEntry->historyItems().size(), 0);

    Group* clonedSubGroup = clonedGroup->children().at(0);
    QVERIFY(clonedSubGroup->uuid() != subGroup->uuid());
    QCOMPARE(clonedSubGroup->name(), QString("SubGroup"));
    QCOMPARE(clonedSubGroup->children().size(), 0);
    QCOMPARE(clonedSubGroup->entries().size(), 1);

    Entry* clonedSubGroupEntry = clonedSubGroup->entries().at(0);
    QVERIFY(clonedSubGroupEntry->uuid() != subGroupEntry->uuid());
    QCOMPARE(clonedSubGroupEntry->title(), QString("SubGroupEntry"));

    QScopedPointer<Group> clonedGroupKeepUuid(originalGroup->clone(Entry::CloneNoFlags));
    QCOMPARE(clonedGroupKeepUuid->entries().at(0)->uuid(), originalGroupEntry->uuid());
    QCOMPARE(clonedGroupKeepUuid->children().at(0)->entries().at(0)->uuid(), subGroupEntry->uuid());

    QScopedPointer<Group> clonedGroupNoFlags(originalGroup->clone(Entry::CloneNoFlags, Group::CloneNoFlags));
    QCOMPARE(clonedGroupNoFlags->entries().size(), 0);
    QVERIFY(clonedGroupNoFlags->uuid() == originalGroup->uuid());

    QScopedPointer<Group> clonedGroupNewUuid(originalGroup->clone(Entry::CloneNoFlags, Group::CloneNewUuid));
    QCOMPARE(clonedGroupNewUuid->entries().size(), 0);
    QVERIFY(clonedGroupNewUuid->uuid() != originalGroup->uuid());

    // Making sure the new modification date is not the same.
    QTest::qSleep(1);

    QScopedPointer<Group> clonedGroupResetTimeInfo(
        originalGroup->clone(Entry::CloneNoFlags, Group::CloneNewUuid | Group::CloneResetTimeInfo));
    QCOMPARE(clonedGroupResetTimeInfo->entries().size(), 0);
    QVERIFY(clonedGroupResetTimeInfo->uuid() != originalGroup->uuid());
    QVERIFY(clonedGroupResetTimeInfo->timeInfo().lastModificationTime()
            != originalGroup->timeInfo().lastModificationTime());
}

void TestGroup::testCopyCustomIcons()
{
    QScopedPointer<Database> dbSource(new Database());
    QScopedPointer<Database> dbTarget(new Database());

    QImage iconImage1(1, 1, QImage::Format_RGB32);
    iconImage1.setPixel(0, 0, qRgb(1, 2, 3));

    QImage iconImage2(1, 1, QImage::Format_RGB32);
    iconImage2.setPixel(0, 0, qRgb(4, 5, 6));

    QScopedPointer<Group> group1(new Group());
    group1->setParent(dbSource->rootGroup());
    QUuid group1Icon = QUuid::createUuid();
    dbSource->metadata()->addCustomIcon(group1Icon, iconImage1);
    group1->setIcon(group1Icon);

    QScopedPointer<Group> group2(new Group());
    group2->setParent(group1.data());
    QUuid group2Icon = QUuid::createUuid();
    dbSource->metadata()->addCustomIcon(group2Icon, iconImage1);
    group2->setIcon(group2Icon);

    QScopedPointer<Entry> entry1(new Entry());
    entry1->setGroup(group2.data());
    QUuid entry1IconOld = QUuid::createUuid();
    dbSource->metadata()->addCustomIcon(entry1IconOld, iconImage1);
    entry1->setIcon(entry1IconOld);

    // add history item
    entry1->beginUpdate();
    QUuid entry1IconNew = QUuid::createUuid();
    dbSource->metadata()->addCustomIcon(entry1IconNew, iconImage1);
    entry1->setIcon(entry1IconNew);
    entry1->endUpdate();

    // test that we don't overwrite icons
    dbTarget->metadata()->addCustomIcon(group2Icon, iconImage2);

    dbTarget->metadata()->copyCustomIcons(group1->customIconsRecursive(), dbSource->metadata());

    Metadata* metaTarget = dbTarget->metadata();

    QCOMPARE(metaTarget->customIcons().size(), 4);
    QVERIFY(metaTarget->containsCustomIcon(group1Icon));
    QVERIFY(metaTarget->containsCustomIcon(group2Icon));
    QVERIFY(metaTarget->containsCustomIcon(entry1IconOld));
    QVERIFY(metaTarget->containsCustomIcon(entry1IconNew));

    QCOMPARE(metaTarget->customIcon(group1Icon).pixel(0, 0), qRgb(1, 2, 3));
    QCOMPARE(metaTarget->customIcon(group2Icon).pixel(0, 0), qRgb(4, 5, 6));
}

void TestGroup::testFindEntry()
{
    QScopedPointer<Database> db(new Database());

    Entry* entry1 = new Entry();
    entry1->setTitle(QString("entry1"));
    entry1->setGroup(db->rootGroup());
    entry1->setUuid(QUuid::createUuid());

    Group* group1 = new Group();
    group1->setName("group1");

    Entry* entry2 = new Entry();

    entry2->setTitle(QString("entry2"));
    entry2->setGroup(group1);
    entry2->setUuid(QUuid::createUuid());

    group1->setParent(db->rootGroup());

    Entry* entry;

    entry = db->rootGroup()->findEntry(entry1->uuid().toRfc4122().toHex());
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry1"));

    entry = db->rootGroup()->findEntry(QString("entry1"));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry1"));

    // We also can find the entry with the leading slash.
    entry = db->rootGroup()->findEntry(QString("/entry1"));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry1"));

    // But two slashes should not be accepted.
    entry = db->rootGroup()->findEntry(QString("//entry1"));
    QVERIFY(entry == nullptr);

    entry = db->rootGroup()->findEntry(entry2->uuid().toRfc4122().toHex());
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry2"));

    entry = db->rootGroup()->findEntry(QString("group1/entry2"));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry2"));

    entry = db->rootGroup()->findEntry(QString("/entry2"));
    QVERIFY(entry == nullptr);

    // We also can find the entry with the leading slash.
    entry = db->rootGroup()->findEntry(QString("/group1/entry2"));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry2"));

    // Should also find the entry only by title.
    entry = db->rootGroup()->findEntry(QString("entry2"));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->title(), QString("entry2"));

    entry = db->rootGroup()->findEntry(QString("invalid/path/to/entry2"));
    QVERIFY(entry == nullptr);

    entry = db->rootGroup()->findEntry(QString("entry27"));
    QVERIFY(entry == nullptr);

    // A valid UUID that does not exist in this database.
    entry = db->rootGroup()->findEntry(QString("febfb01ebcdf9dbd90a3f1579dc75281"));
    QVERIFY(entry == nullptr);

    // An invalid UUID.
    entry = db->rootGroup()->findEntry(QString("febfb01ebcdf9dbd90a3f1579dc"));
    QVERIFY(entry == nullptr);
}

void TestGroup::testFindGroupByPath()
{
    QScopedPointer<Database> db(new Database());

    Group* group1 = new Group();
    group1->setName("group1");
    group1->setParent(db->rootGroup());

    Group* group2 = new Group();
    group2->setName("group2");
    group2->setParent(group1);

    Group* group;

    group = db->rootGroup()->findGroupByPath("/");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), db->rootGroup()->uuid());

    // We also accept it if the leading slash is missing.
    group = db->rootGroup()->findGroupByPath("");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), db->rootGroup()->uuid());

    group = db->rootGroup()->findGroupByPath("/group1/");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group1->uuid());

    // We also accept it if the leading slash is missing.
    group = db->rootGroup()->findGroupByPath("group1/");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group1->uuid());

    // Too many slashes at the end
    group = db->rootGroup()->findGroupByPath("group1//");
    QVERIFY(group == nullptr);

    // Missing a slash at the end.
    group = db->rootGroup()->findGroupByPath("/group1");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group1->uuid());

    // Too many slashes at the start
    group = db->rootGroup()->findGroupByPath("//group1");
    QVERIFY(group == nullptr);

    group = db->rootGroup()->findGroupByPath("/group1/group2/");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group2->uuid());

    // We also accept it if the leading slash is missing.
    group = db->rootGroup()->findGroupByPath("group1/group2/");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group2->uuid());

    group = db->rootGroup()->findGroupByPath("group1/group2");
    QVERIFY(group != nullptr);
    QCOMPARE(group->uuid(), group2->uuid());

    group = db->rootGroup()->findGroupByPath("invalid");
    QVERIFY(group == nullptr);
}

void TestGroup::testPrint()
{
    QScopedPointer<Database> db(new Database());

    QString output = db->rootGroup()->print();
    QCOMPARE(output, QString("[empty]\n"));

    output = db->rootGroup()->print(true);
    QCOMPARE(output, QString("[empty]\n"));

    Entry* entry1 = new Entry();
    entry1->setTitle(QString("entry1"));
    entry1->setGroup(db->rootGroup());
    entry1->setUuid(QUuid::createUuid());

    output = db->rootGroup()->print();
    QCOMPARE(output, QString("entry1\n"));

    Group* group1 = new Group();
    group1->setName("group1");

    Entry* entry2 = new Entry();

    entry2->setTitle(QString("entry2"));
    entry2->setGroup(group1);
    entry2->setUuid(QUuid::createUuid());

    group1->setParent(db->rootGroup());

    output = db->rootGroup()->print();
    QVERIFY(output.contains(QString("entry1\n")));
    QVERIFY(output.contains(QString("group1/\n")));
    QVERIFY(!output.contains(QString("  entry2\n")));

    output = db->rootGroup()->print(true);
    QVERIFY(output.contains(QString("entry1\n")));
    QVERIFY(output.contains(QString("group1/\n")));
    QVERIFY(output.contains(QString("  entry2\n")));

    output = group1->print();
    QVERIFY(!output.contains(QString("group1/\n")));
    QVERIFY(output.contains(QString("entry2\n")));
}

void TestGroup::testLocate()
{
    Database* db = new Database();

    Entry* entry1 = new Entry();
    entry1->setTitle("entry1");
    entry1->setGroup(db->rootGroup());

    Entry* entry2 = new Entry();
    entry2->setTitle("entry2");
    entry2->setGroup(db->rootGroup());

    Group* group1 = new Group();
    group1->setName("group1");
    group1->setParent(db->rootGroup());

    Group* group2 = new Group();
    group2->setName("group2");
    group2->setParent(group1);

    Entry* entry3 = new Entry();
    entry3->setTitle("entry3");
    entry3->setGroup(group1);

    Entry* entry43 = new Entry();
    entry43->setTitle("entry43");
    entry43->setGroup(group1);

    Entry* google = new Entry();
    google->setTitle("Google");
    google->setGroup(group2);

    QStringList results = db->rootGroup()->locate("entry");
    QVERIFY(results.size() == 4);
    QVERIFY(results.contains("/group1/entry43"));

    results = db->rootGroup()->locate("entry1");
    QVERIFY(results.size() == 1);
    QVERIFY(results.contains("/entry1"));

    results = db->rootGroup()->locate("Entry1");
    QVERIFY(results.size() == 1);
    QVERIFY(results.contains("/entry1"));

    results = db->rootGroup()->locate("invalid");
    QVERIFY(results.size() == 0);

    results = db->rootGroup()->locate("google");
    QVERIFY(results.size() == 1);
    QVERIFY(results.contains("/group1/group2/Google"));

    results = db->rootGroup()->locate("group1");
    QVERIFY(results.size() == 3);
    QVERIFY(results.contains("/group1/entry3"));
    QVERIFY(results.contains("/group1/entry43"));
    QVERIFY(results.contains("/group1/group2/Google"));

    delete db;
}

void TestGroup::testAddEntryWithPath()
{
    Database* db = new Database();

    Group* group1 = new Group();
    group1->setName("group1");
    group1->setParent(db->rootGroup());

    Group* group2 = new Group();
    group2->setName("group2");
    group2->setParent(group1);

    Entry* entry = db->rootGroup()->addEntryWithPath("entry1");
    QVERIFY(entry != nullptr);
    QVERIFY(!entry->uuid().isNull());

    entry = db->rootGroup()->addEntryWithPath("entry1");
    QVERIFY(entry == nullptr);

    entry = db->rootGroup()->addEntryWithPath("/entry1");
    QVERIFY(entry == nullptr);

    entry = db->rootGroup()->addEntryWithPath("entry2");
    QVERIFY(entry != nullptr);
    QVERIFY(entry->title() == "entry2");
    QVERIFY(!entry->uuid().isNull());

    entry = db->rootGroup()->addEntryWithPath("/entry3");
    QVERIFY(entry != nullptr);
    QVERIFY(entry->title() == "entry3");
    QVERIFY(!entry->uuid().isNull());

    entry = db->rootGroup()->addEntryWithPath("/group1/entry4");
    QVERIFY(entry != nullptr);
    QVERIFY(entry->title() == "entry4");
    QVERIFY(!entry->uuid().isNull());

    entry = db->rootGroup()->addEntryWithPath("/group1/group2/entry5");
    QVERIFY(entry != nullptr);
    QVERIFY(entry->title() == "entry5");
    QVERIFY(!entry->uuid().isNull());

    entry = db->rootGroup()->addEntryWithPath("/group1/invalid_group/entry6");
    QVERIFY(entry == nullptr);

    delete db;
}
