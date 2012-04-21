/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "crypto/Crypto.h"

void TestGroup::initTestCase()
{
    qRegisterMetaType<Entry*>("Entry*");
    qRegisterMetaType<Group*>("Group*");
    Crypto::init();
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
    QVERIFY(g1->children().at(0) == g2);
    QVERIFY(g1->children().at(1) == g3);
    QVERIFY(g3->children().contains(g4));

    QSignalSpy spy(db, SIGNAL(groupDataChanged(Group*)));
    g2->setName("test");
    g4->setName("test");
    g3->setName("test");
    g1->setName("test");
    g3->setIcon(Uuid::random());
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
    QPointer<Group> root = db->rootGroup();

    Group* g1 = new Group();
    Group* g2 = new Group();
    g1->setParent(root);
    g2->setParent(root);

    QSignalSpy spyAboutToAdd(db, SIGNAL(groupAboutToAdd(Group*,int)));
    QSignalSpy spyAdded(db, SIGNAL(groupAdded()));
    QSignalSpy spyAboutToRemove(db, SIGNAL(groupAboutToRemove(Group*)));
    QSignalSpy spyRemoved(db, SIGNAL(groupRemoved()));

    g2->setParent(root, 0);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    delete db;

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
    Database* db = new Database();
    Group* groupRoot = db->rootGroup();
    Group* groupChild = new Group();
    Group* groupChildChild = new Group();
    groupRoot->setObjectName("groupRoot");
    groupChild->setObjectName("groupChild");
    groupChildChild->setObjectName("groupChildChild");
    groupChild->setParent(groupRoot);
    groupChildChild->setParent(groupChild);
    QSignalSpy spyAboutToRemove(db, SIGNAL(groupAboutToRemove(Group*)));
    QSignalSpy spyRemoved(db, SIGNAL(groupRemoved()));

    delete groupChild;
    QVERIFY(groupRoot->children().isEmpty());
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);
    delete db;


    Group* group = new Group();
    Entry* entry = new Entry();
    entry->setGroup(group);
    QSignalSpy spyEntryAboutToRemove(group, SIGNAL(entryAboutToRemove(Entry*)));
    QSignalSpy spyEntryRemoved(group, SIGNAL(entryRemoved()));

    delete entry;
    QVERIFY(group->entries().isEmpty());
    QCOMPARE(spyEntryAboutToRemove.count(), 1);
    QCOMPARE(spyEntryRemoved.count(), 1);
    delete group;

    Database* db2 = new Database();
    Group* groupRoot2 = db2->rootGroup();
    Group* group2 = new Group();
    group2->setParent(groupRoot2);
    Entry* entry2 = new Entry();
    entry2->setGroup(group2);
    QSignalSpy spyEntryAboutToRemove2(group2, SIGNAL(entryAboutToRemove(Entry*)));
    QSignalSpy spyEntryRemoved2(group2, SIGNAL(entryRemoved()));

    delete group2;
    QCOMPARE(spyEntryAboutToRemove2.count(), 0);
    QCOMPARE(spyEntryRemoved2.count(), 0);
    delete db2;
}

KEEPASSX_QTEST_CORE_MAIN(TestGroup)
