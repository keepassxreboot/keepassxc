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

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
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

    QScopedPointer<Database> db(new Database());
    auto* root = db->rootGroup();
    QSignalSpy spyModified(db.data(), SIGNAL(modifiedImmediate()));

    db->setKey(compositeKey);
    QCOMPARE(spyModified.count(), ++spyCount);

    auto* group1 = new Group();
    group1->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    auto* group2 = new Group();
    group2->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    group2->setParent(root, 0);
    QCOMPARE(spyModified.count(), ++spyCount);

    auto* entry1 = new Entry();
    entry1->setGroup(group1);
    QCOMPARE(spyModified.count(), ++spyCount);

    QScopedPointer<Database> db2(new Database());
    auto* root2 = db2->rootGroup();
    QSignalSpy spyModified2(db2.data(), SIGNAL(modifiedImmediate()));

    group1->setParent(root2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    entry1->setTitle("test");
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    auto* entry2 = new Entry();
    entry2->setGroup(group2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);

    entry2->setGroup(root2);
    QCOMPARE(spyModified.count(), ++spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    entry2->setTitle("test2");
    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), ++spyCount2);

    auto* group3 = new Group();
    group3->setParent(root);
    QCOMPARE(spyModified.count(), ++spyCount);

    auto* group4 = new Group();
    group4->setParent(group3);
    QCOMPARE(spyModified.count(), ++spyCount);

    delete group4;
    QCOMPARE(spyModified.count(), ++spyCount);

    delete entry2;
    QCOMPARE(spyModified2.count(), ++spyCount2);

    QCOMPARE(spyModified.count(), spyCount);
    QCOMPARE(spyModified2.count(), spyCount2);
}

void TestModified::testGroupSets()
{
    int spyCount = 0;
    QScopedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    auto* group = new Group();
    group->setParent(root);

    QSignalSpy spyModified(db.data(), SIGNAL(modifiedImmediate()));

    root->setUuid(QUuid::createUuid());
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

    root->setIcon(QUuid::createUuid());
    QCOMPARE(spyModified.count(), ++spyCount);
    root->setIcon(root->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);


    group->setUuid(QUuid::createUuid());
    QCOMPARE(spyModified.count(), ++spyCount);
    group->setUuid(group->uuid());
    QCOMPARE(spyModified.count(), spyCount);

    group->setName("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    group->setName(group->name());
    QCOMPARE(spyModified.count(), spyCount);

    group->setNotes("test");
    QCOMPARE(spyModified.count(), ++spyCount);
    group->setNotes(group->notes());
    QCOMPARE(spyModified.count(), spyCount);

    group->setIcon(1);
    QCOMPARE(spyModified.count(), ++spyCount);
    group->setIcon(group->iconNumber());
    QCOMPARE(spyModified.count(), spyCount);

    group->setIcon(QUuid::createUuid());
    QCOMPARE(spyModified.count(), ++spyCount);
    group->setIcon(group->iconUuid());
    QCOMPARE(spyModified.count(), spyCount);
}

void TestModified::testEntrySets()
{
    int spyCount = 0;
    QScopedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    auto* group = new Group();
    group->setParent(root);
    auto* entry = new Entry();
    entry->setGroup(group);

    QSignalSpy spyModified(db.data(), SIGNAL(modifiedImmediate()));

    entry->setUuid(QUuid::createUuid());
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

    entry->setIcon(QUuid::createUuid());
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

    entry->setExpiryTime(QDateTime::currentDateTimeUtc().addYears(1));
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
}

void TestModified::testHistoryItems()
{
    QScopedPointer<Entry> entry(new Entry());
    QDateTime created = entry->timeInfo().creationTime();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle("a");
    entry->setTags("a");
    QScopedPointer<EntryAttributes> attributes(new EntryAttributes());
    attributes->copyCustomKeysFrom(entry->attributes());

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
    auto* historyEntry = entry->historyItems().at(historyItemsSize - 1);
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
    entry->attributes()->copyCustomKeysFrom(attributes.data());
    entry->endUpdate();
    QCOMPARE(entry->historyItems().size(), ++historyItemsSize);
    QVERIFY(!entry->historyItems().at(historyItemsSize - 1)->attributes()->keys().contains("k"));

    QScopedPointer<Database> db(new Database());
    auto* root = db->rootGroup();
    db->metadata()->setHistoryMaxItems(3);
    db->metadata()->setHistoryMaxSize(-1);

    auto* entry2 = new Entry();
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

    auto* historyEntry2 = entry2->historyItems().at(0);
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

    const int historyMaxSize = 19000;

    db->metadata()->setHistoryMaxItems(-1);
    db->metadata()->setHistoryMaxSize(historyMaxSize);

    const QString key("test");
    entry2->beginUpdate();
    entry2->attachments()->set(key, QByteArray(18000, 'X'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 18000 + key.size());
    QCOMPARE(entry2->historyItems().size(), 1);

    historyEntry2 = entry2->historyItems().at(0);
    QCOMPARE(historyEntry2->title(), QString("7"));

    entry2->beginUpdate();
    entry2->setTitle("8");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 2);

    entry2->beginUpdate();
    entry2->attachments()->remove(key);
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 0);
    QCOMPARE(entry2->historyItems().size(), 1);

    entry2->beginUpdate();
    entry2->attachments()->set("test2", QByteArray(6000, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 6000 + key.size() + 1);
    QCOMPARE(entry2->historyItems().size(), 2);

    entry2->beginUpdate();
    entry2->attachments()->set("test3", QByteArray(6000, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 12000 + (key.size() + 1) * 2);
    QCOMPARE(entry2->historyItems().size(), 2);

    entry2->beginUpdate();
    entry2->attachments()->set("test4", QByteArray(6000, 'c'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 18000 + (key.size() + 1) * 3);
    QCOMPARE(entry2->historyItems().size(), 3);

    entry2->beginUpdate();
    entry2->attachments()->set("test5", QByteArray(6000, 'd'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 24000 + (key.size() + 1) * 4);
    QCOMPARE(entry2->historyItems().size(), 1);
}

void TestModified::testHistoryMaxSize()
{
    QScopedPointer<Database> db(new Database());
    const QString key("test");

    auto entry1 = new Entry();
    entry1->setGroup(db->rootGroup());
    QCOMPARE(entry1->historyItems().size(), 0);

    const int reservedSize1 = entry1->attributes()->attributesSize();
    db->metadata()->setHistoryMaxItems(-1);
    db->metadata()->setHistoryMaxSize(18000 + key.size() * 3 + reservedSize1 * 4);

    entry1->beginUpdate();
    entry1->attachments()->set(key, QByteArray(6000, 'a'));
    entry1->endUpdate();
    QCOMPARE(entry1->attachments()->attachmentsSize(), 6000 + key.size());
    QCOMPARE(entry1->historyItems().size(), 1);

    entry1->beginUpdate();
    entry1->attachments()->set(key, QByteArray(6000, 'b'));
    entry1->endUpdate();
    QCOMPARE(entry1->attachments()->attachmentsSize(), 6000 + key.size());
    QCOMPARE(entry1->historyItems().size(), 2);

    entry1->beginUpdate();
    entry1->attachments()->set(key, QByteArray(6000, 'c'));
    entry1->endUpdate();
    QCOMPARE(entry1->attachments()->attachmentsSize(), 6000 + key.size());
    QCOMPARE(entry1->historyItems().size(), 3);

    entry1->beginUpdate();
    entry1->attachments()->set(key, QByteArray(6000, 'd'));
    entry1->endUpdate();
    QCOMPARE(entry1->attachments()->attachmentsSize(), 6000 + key.size());
    QCOMPARE(entry1->historyItems().size(), 4);

    auto entry2 = new Entry();
    entry2->setGroup(db->rootGroup());
    QCOMPARE(entry2->historyItems().size(), 0);

    const int historyMaxSize = 17000;
    const int reservedSize2 = entry2->attributes()->attributesSize();
    db->metadata()->setHistoryMaxSize(historyMaxSize);

    entry2->beginUpdate();
    entry2->attachments()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), historyMaxSize - reservedSize2 + 1);
    QCOMPARE(entry2->historyItems().size(), 1);

    // history size overflow
    entry2->beginUpdate();
    entry2->attachments()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attachments()->remove(key);
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 0);
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attachments()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), historyMaxSize - reservedSize2 + 1);
    QCOMPARE(entry2->historyItems().size(), 1);

    // history size overflow
    entry2->beginUpdate();
    entry2->attachments()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attachments()->remove(key);
    entry2->endUpdate();
    QCOMPARE(entry2->attachments()->attachmentsSize(), 0);
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->setTags(QByteArray(historyMaxSize - reservedSize2 + 1, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->tags().size(), historyMaxSize - reservedSize2 + 1);
    QCOMPARE(entry2->historyItems().size(), 1);

    // history size overflow
    entry2->beginUpdate();
    entry2->setTags(QByteArray(historyMaxSize - reservedSize2 + 1, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->setTags("");
    entry2->endUpdate();
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attributes()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'a'));
    entry2->endUpdate();
    QCOMPARE(entry2->attributes()->attributesSize(), historyMaxSize + 1);
    QCOMPARE(entry2->historyItems().size(), 1);

    // history size overflow
    entry2->beginUpdate();
    entry2->attributes()->set(key, QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'b'));
    entry2->endUpdate();
    QCOMPARE(entry2->attributes()->attributesSize(), historyMaxSize + 1);
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    entry2->attributes()->remove(key);
    entry2->endUpdate();
    QCOMPARE(entry2->attributes()->attributesSize(), reservedSize2);
    QCOMPARE(entry2->historyItems().size(), 0);

    entry2->beginUpdate();
    AutoTypeAssociations::Association association;
    association.window = key;
    association.sequence = QByteArray(historyMaxSize - key.size() - reservedSize2 + 1, 'a');
    entry2->autoTypeAssociations()->add(association);
    entry2->endUpdate();
    QCOMPARE(entry2->autoTypeAssociations()->associationsSize(), historyMaxSize - reservedSize2 + 1);
    QCOMPARE(entry2->historyItems().size(), 1);

    entry2->beginUpdate();
    entry2->autoTypeAssociations()->remove(0);
    entry2->endUpdate();
    QCOMPARE(entry2->autoTypeAssociations()->associationsSize(), 0);
    QCOMPARE(entry2->historyItems().size(), 0);
}

void TestModified::testCustomData()
{
    int spyCount = 0;
    QScopedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    auto* group = new Group();
    group->setParent(root);
    auto* entry = new Entry();
    entry->setGroup(group);

    QSignalSpy spyModified(db.data(), SIGNAL(modifiedImmediate()));

    db->metadata()->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), ++spyCount);
    db->metadata()->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), spyCount);

    entry->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), ++spyCount);
    entry->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), spyCount);

    group->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), ++spyCount);
    group->customData()->set("Key", "Value");
    QCOMPARE(spyModified.count(), spyCount);
}
