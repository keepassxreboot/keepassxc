/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "TestModified.h"

#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "crypto/Crypto.h"

void TestModified::initTestCase()
{
    Crypto::init();
}

void TestModified::testSignals()
{
    int spyCount = 0;
    int spyCount2 = 0;

    CompositeKey* compositeKey = new CompositeKey();

    Database* db = new Database();
    QPointer<Group> root = db->rootGroup();
    QSignalSpy spyModified(db, SIGNAL(modified()));

    db->setKey(*compositeKey);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    Group* g1 = new Group();
    g1->setParent(root);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    Group* g2 = new Group();
    g2->setParent(root);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    QPointer<Entry> entry1 = new Entry();
    entry1->setGroup(g1);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    Database* db2 = new Database();
    QPointer<Group> root2 = db2->rootGroup();
    QSignalSpy spyModified2(db2, SIGNAL(modified()));

    g1->setParent(root2);
    spyCount++;
    spyCount2++;
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    entry1->setTitle("test");
    spyCount2++;
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    QPointer<Entry> entry2 = new Entry();
    entry2->setGroup(g2);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    entry2->setGroup(root2);
    spyCount++;
    spyCount2++;
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    entry2->setTitle("test2");
    spyCount2++;
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    Group* g3 = new Group();
    g3->setParent(root);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    Group* g4 = new Group();
    g4->setParent(g3);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    QCOMPARE(spyModified2.count(), spyCount2);
}

void TestModified::testGroupSets()
{
    int spyCount = 0;
    Database* db = new Database();
    QPointer<Group> root = db->rootGroup();

    Group* g = new Group();
    g->setParent(root);

    QSignalSpy spyModified(db, SIGNAL(modified()));

    root->setUuid(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    root->setUuid(root->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    root->setName("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    root->setName(root->name());
    QCOMPARE(spyModified.count(), spyCount);

    root->setNotes("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    root->setNotes(root->notes());
    QCOMPARE(spyModified.count(), spyCount);

    root->setIcon(1);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    root->setIcon(root->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    root->setIcon(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    root->setIcon(root->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);


    g->setUuid(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    g->setUuid(g->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    g->setName("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    g->setName(g->name());
    QCOMPARE(spyModified.count(), spyCount);

    g->setNotes("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    g->setNotes(g->notes());
    QCOMPARE(spyModified.count(), spyCount);

    g->setIcon(1);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    g->setIcon(g->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    g->setIcon(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    g->setIcon(g->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);
}

void TestModified::testEntrySets()
{
    int spyCount = 0;
    Database* db = new Database();
    QPointer<Group> root = db->rootGroup();

    Group* g = new Group();
    g->setParent(root);
    QPointer<Entry> entry = new Entry();
    entry->setGroup(g);

    QSignalSpy spyModified(db, SIGNAL(modified()));

    entry->setUuid(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setUuid(entry->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setTitle("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setTitle(entry->title());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setUrl("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setUrl(entry->url());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setUsername("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setUsername(entry->username());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setPassword("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setPassword(entry->password());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setNotes("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setNotes(entry->notes());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setIcon(1);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setIcon(entry->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setIcon(Uuid::random());
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setIcon(entry->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setTags("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setTags(entry->tags());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setExpires(true);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setExpires(entry->timeInfo().expires());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setExpiryTime(QDateTime::currentDateTimeUtc().addYears(1));
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setExpiryTime(entry->timeInfo().expiryTime());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setAutoTypeEnabled(false);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setAutoTypeEnabled(entry->autoTypeEnabled());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setAutoTypeObfuscation(1);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setAutoTypeObfuscation(entry->autoTypeObfuscation());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setDefaultAutoTypeSequence("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setDefaultAutoTypeSequence(entry->defaultAutoTypeSequence());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setForegroundColor(Qt::red);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setForegroundColor(entry->foregroundColor());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setBackgroundColor(Qt::red);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setBackgroundColor(entry->backgroundColor());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setOverrideUrl("test");
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->setOverrideUrl(entry->overrideUrl());
    QCOMPARE(spyModified.count(), spyCount);

    entry->attributes()->set("test key", "test value", false);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->attributes()->set("test key", entry->attributes()->value("test key"), false);
    QCOMPARE(spyModified.count(), spyCount);
    entry->attributes()->set("test key", entry->attributes()->value("test key"), true);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->attributes()->set("test key", "new test value", true);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);

    entry->attributes()->set("test key2", "test value2", true);
    spyCount++;
    QCOMPARE(spyModified.count(), spyCount);
    entry->attributes()->set("test key2", entry->attributes()->value("test key2"), true);
    QCOMPARE(spyModified.count(), spyCount);
}

KEEPASSX_QTEST_CORE_MAIN(TestModified)
