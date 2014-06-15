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

#include <QSignalSpy>
#include <QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestModified)

void TestModified::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestModified::testSignals()
{
    int spyCount = 0;
    int spyCount2 = 0;

    CompositeKey compositeKey;

    Database* db = new Database();
    Group* root = db->rootGroup();
    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    db->setKey(compositeKey);
    QCOMPARE(spyModified.count(), ++spyCount);

    Group* g1 = new Group();
    g1->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    Group* g2 = new Group();
    g2->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    g2->setParent(root, 0);
    QCOMPARE(spyModified.count(), ++spyCount);

    Entry* entry1 = new Entry();
    entry1->setGroup(g1);
    QCOMPARE(spyModified.count(), ++spyCount);

    Database* db2 = new Database();
    Group* root2 = db2->rootGroup();
    QSignalSpy spyModified2(db2, SIGNAL(modifiedImmediate()));

    g1->setParent(root2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    entry1->setTitle("test");
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    Entry* entry2 = new Entry();
    entry2->setGroup(g2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    entry2->setGroup(root2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    entry2->setTitle("test2");
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    Group* g3 = new Group();
    g3->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    Group* g4 = new Group();
    g4->setParent(g3);
    QCOMPARE(spyModified.count(), ++spyCount);

    delete g4;
    QCOMPARE(spyModified.count(), ++spyCount);

    delete entry2;
    QCOMPARE(spyModified2.count(), ++spyCount2);

    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    delete db;
    delete db2;
}

void TestModified::testGroupSets()
{
    int spyCount = 0;
    Database* db = new Database();
    Group* root = db->rootGroup();

    Group* g = new Group();
    g->setParent(root);

    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    root->setUuid(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setUuid(root->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    root->setName("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setName(root->name());
    QCOMPARE(spyModified.count(), spyCount);

    root->setNotes("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setNotes(root->notes());
    QCOMPARE(spyModified.count(), spyCount);

    root->setIcon(1);
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setIcon(root->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    root->setIcon(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setIcon(root->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);


    g->setUuid(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    g->setUuid(g->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    g->setName("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    g->setName(g->name());
    QCOMPARE(spyModified.count(), spyCount);

    g->setNotes("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    g->setNotes(g->notes());
    QCOMPARE(spyModified.count(), spyCount);

    g->setIcon(1);
    QCOMPARE(spyModified.count(), ++spyCount);
    g->setIcon(g->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    g->setIcon(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    g->setIcon(g->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);

    delete db;
}

void TestModified::testEntrySets()
{
    int spyCount = 0;
    Database* db = new Database();
    Group* root = db->rootGroup();

    Group* g = new Group();
    g->setParent(root);
    Entry* entry = new Entry();
    entry->setGroup(g);

    QSignalSpy spyModified(db, SIGNAL(modifiedImmediate()));

    entry->setUuid(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setUuid(entry->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setTitle("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setTitle(entry->title());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setUrl("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setUrl(entry->url());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setUsername("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setUsername(entry->username());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setPassword("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setPassword(entry->password());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setNotes("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setNotes(entry->notes());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setIcon(1);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setIcon(entry->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setIcon(Uuid::random());
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setIcon(entry->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setTags("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setTags(entry->tags());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setExpires(true);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setExpires(entry->timeInfo().expires());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setExpiryTime(Tools::currentDateTimeUtc().addYears(1));
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setExpiryTime(entry->timeInfo().expiryTime());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setAutoTypeEnabled(false);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setAutoTypeEnabled(entry->autoTypeEnabled());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setAutoTypeObfuscation(1);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setAutoTypeObfuscation(entry->autoTypeObfuscation());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setDefaultAutoTypeSequence("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setDefaultAutoTypeSequence(entry->defaultAutoTypeSequence());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setForegroundColor(Qt::red);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setForegroundColor(entry->foregroundColor());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setBackgroundColor(Qt::red);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setBackgroundColor(entry->backgroundColor());
    QCOMPARE(spyModified.count(), spyCount);

    entry->setOverrideUrl("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->setOverrideUrl(entry->overrideUrl());
    QCOMPARE(spyModified.count(), spyCount);

    entry->attributes()->set("test key", "test value", false);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->attributes()->set("test key", entry->attributes()->value("test key"), false);
    QCOMPARE(spyModified.count(), spyCount);
    entry->attributes()->set("test key", entry->attributes()->value("test key"), true);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->attributes()->set("test key", "new test value", true);
    QCOMPARE(spyModified.count(), ++spyCount);

    entry->attributes()->set("test key2", "test value2", true);
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->attributes()->set("test key2", entry->attributes()->value("test key2"), true);
    QCOMPARE(spyModified.count(), spyCount);

    delete db;
}

void TestModified::testHistoryItem()
{
    Entry* entry = new Entry();
    QDateTime created = entry->timeInfo().creationTime();
    entry->setUuid(Uuid::random());
    entry->setTitle("a");
    entry->setTags("a");
    EntryAttributes* attributes = new EntryAttributes();
    attributes->copyCustomKeysFrom(entry->attributes());

    Entry* historyEntry;

    int historyItemsSize = 0;

    entry->beginUpdate();
    entry->setTitle("a");
    entry->setTags("a");
    entry->setOverrideUrl("");
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), historyItemsSize);

    QDateTime modified = entry->timeInfo().lastModificationTime();
    QTest::qSleep(10);
    entry->beginUpdate();
    entry->setTitle("b");
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), ++historyItemsSize);
    historyEntry = entry->historyItems().at(historyItemsSize - 1);
    QCOMPARE(historyEntry->title(), QString("a"));
    QCOMPARE(historyEntry->uuid(), entry->uuid());
    QCOMPARE(historyEntry->tags(), entry->tags());
    QCOMPARE(historyEntry->overrideUrl(), entry->overrideUrl());
    QCOMPARE(historyEntry->timeInfo().creationTime(), created);
    QCOMPARE(historyEntry->timeInfo().lastModificationTime(), modified);
    QCOMPARE(historyEntry->historyItems().size(), 0);

    entry->beginUpdate();
    entry->setTags("b");
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), ++historyItemsSize);
    QCOMPARE(entry->historyItems().at(historyItemsSize - 1)->tags(), QString("a"));

    entry->beginUpdate();
    entry->attachments()->set("test", QByteArray("value"));
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), ++historyItemsSize);
    QCOMPARE(entry->historyItems().at(historyItemsSize - 1)->attachments()->keys().size(), 0);

    attributes->set("k", "myvalue");
    entry->beginUpdate();
    entry->attributes()->copyCustomKeysFrom(attributes);
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), ++historyItemsSize);
    QVERIFY(!entry->historyItems().at(historyItemsSize - 1)->attributes()->keys().contains("k"));

    delete attributes;
    delete entry;

    Database* db = new Database();
    Group* root = db->rootGroup();
    db->metadata()->setHistoryMaxItems(3);
    db->metadata()->setHistoryMaxSize(-1);

    Entry* historyEntry2;
    Entry* entry2 = new Entry();
    entry2->setGroup(root);
    entry2->beginUpdate();
    entry2->setTitle("1");
    entry2->endUpdate();

    entry2->beginUpdate();
    entry2->setTitle("2");
    entry2->endUpdate();
    entry2->beginUpdate();
    entry2->setTitle("3");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 3);

    entry2->beginUpdate();
    entry2->setTitle("4");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 3);

    db->metadata()->setHistoryMaxItems(1);

    entry2->beginUpdate();
    entry2->setTitle("5");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 1);

    historyEntry2 = entry2->historyItems().at(0);
    QCOMPARE(historyEntry2->title(), QString("4"));

    db->metadata()->setHistoryMaxItems(-1);

    for (int i = 0; i < 20; i++) {
        entry2->beginUpdate();
        entry2->setTitle("6");
        entry2->endUpdate();
        entry2->beginUpdate();
        entry2->setTitle("6b");
        entry2->endUpdate();
    }
    QCOMPARE(entry2->historyItems().size(), 41);

    db->metadata()->setHistoryMaxItems(0);

    entry2->beginUpdate();
    entry2->setTitle("7");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    db->metadata()->setHistoryMaxItems(-1);
    db->metadata()->setHistoryMaxSize(17000);

    entry2->beginUpdate();
    entry2->attachments()->set("test", QByteArray(18000, 'X'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 1);

    historyEntry2 = entry2->historyItems().at(0);
    QCOMPARE(historyEntry2->title(), QString("7"));

    entry2->beginUpdate();
    entry2->setTitle("8");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 2);

    entry2->beginUpdate();
    entry2->attachments()->remove("test");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attachments()->set("test2", QByteArray(6000, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 1);

    entry2->beginUpdate();
    entry2->attachments()->set("test3", QByteArray(6000, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 2);

    entry2->beginUpdate();
    entry2->attachments()->set("test4", QByteArray(6000, 'c'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 3);

    entry2->beginUpdate();
    entry2->attachments()->set("test5", QByteArray(6000, 'd'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 4);

    Entry* entry3 = new Entry();
    entry3->setGroup(root);
    QCOMPARE(entry3->historyItems().size(), 0);

    entry3->beginUpdate();
    entry3->attachments()->set("test", QByteArray(6000, 'a'));
    entry3->endUpdate();
    QCOMPARE(entry3->historyItems().size(), 1);

    entry3->beginUpdate();
    entry3->attachments()->set("test", QByteArray(6000, 'b'));
    entry3->endUpdate();
    QCOMPARE(entry3->historyItems().size(), 2);

    entry3->beginUpdate();
    entry3->attachments()->set("test", QByteArray(6000, 'c'));
    entry3->endUpdate();
    QCOMPARE(entry3->historyItems().size(), 3);

    entry3->beginUpdate();
    entry3->attachments()->set("test", QByteArray(6000, 'd'));
    entry3->endUpdate();
    QCOMPARE(entry3->historyItems().size(), 2);

    delete db;
}
