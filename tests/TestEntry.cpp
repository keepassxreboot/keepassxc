/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "TestEntry.h"

#include <QtTest/QTest>

#include "tests.h"
#include "core/Entry.h"

void TestEntry::testHistoryItemDeletion()
{
    Entry* entry = new Entry();
    QPointer<Entry> historyEntry = new Entry();

    entry->addHistoryItem(historyEntry);
    QCOMPARE(entry->historyItems().size(), 1);

    QList<Entry*> historyEntriesToRemove;
    historyEntriesToRemove.append(historyEntry);
    entry->removeHistoryItems(historyEntriesToRemove);
    QCOMPARE(entry->historyItems().size(), 0);
    QVERIFY(historyEntry.isNull());

    delete entry;
}
void TestEntry::testCopyDataFrom()
{
    Entry* entry = new Entry();

    entry->setTitle("testtitle");
    entry->attributes()->set("attr1", "abc");
    entry->attributes()->set("attr2", "def");

    entry->attachments()->set("test", "123");
    entry->attachments()->set("test2", "456");

    AutoTypeAssociations::Association assoc;
    assoc.window = "1";
    assoc.sequence = "2";
    entry->autoTypeAssociations()->add(assoc);
    assoc.window = "3";
    assoc.sequence = "4";
    entry->autoTypeAssociations()->add(assoc);

    Entry* entry2 = new Entry();
    entry2->copyDataFrom(entry);
    delete entry;

    QCOMPARE(entry2->title(), QString("testtitle"));
    QCOMPARE(entry2->attributes()->value("attr1"), QString("abc"));
    QCOMPARE(entry2->attributes()->value("attr2"), QString("def"));

    QCOMPARE(entry2->attachments()->keys().size(), 2);
    QCOMPARE(entry2->attachments()->value("test"), QByteArray("123"));
    QCOMPARE(entry2->attachments()->value("test2"), QByteArray("456"));

    QCOMPARE(entry2->autoTypeAssociations()->size(), 2);
    QCOMPARE(entry2->autoTypeAssociations()->get(0).window, QString("1"));
    QCOMPARE(entry2->autoTypeAssociations()->get(1).window, QString("3"));
}

QTEST_GUILESS_MAIN(TestEntry)
