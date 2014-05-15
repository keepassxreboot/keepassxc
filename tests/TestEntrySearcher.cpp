/*
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "TestEntrySearcher.h"

#include <QTest>

#include "tests.h"
#include "core/EntrySearcher.h"
#include "core/Group.h"

QTEST_GUILESS_MAIN(TestEntrySearcher)

void TestEntrySearcher::initTestCase()
{
    groupRoot = new Group();
}

void TestEntrySearcher::cleanupTestCase()
{
    delete groupRoot;
}

void TestEntrySearcher::testSearch()
{
    Group* group1 = new Group();
    Group* group2 = new Group();
    Group* group3 = new Group();

    group1->setParent(groupRoot);
    group2->setParent(groupRoot);
    group3->setParent(groupRoot);

    Group* group11 = new Group();

    group11->setParent(group1);

    Group* group21 = new Group();
    Group* group211 = new Group();
    Group* group2111 = new Group();

    group21->setParent(group2);
    group211->setParent(group21);
    group2111->setParent(group211);

    group1->setSearchingEnabled(Group::Disable);
    group11->setSearchingEnabled(Group::Enable);

    Entry* eRoot = new Entry();
    eRoot->setNotes("test search term test");
    eRoot->setGroup(groupRoot);

    Entry* eRoot2 = new Entry();
    eRoot2->setNotes("test term test");
    eRoot2->setGroup(groupRoot);

    Entry* e1 = new Entry();
    e1->setNotes("test search term test");
    e1->setGroup(group1);

    Entry* e11 = new Entry();
    e11->setNotes("test search term test");
    e11->setGroup(group11);

    Entry* e2111 = new Entry();
    e2111->setNotes("test search term test");
    e2111->setGroup(group2111);

    Entry* e2111b = new Entry();
    e2111b->setNotes("test search test");
    e2111b->setGroup(group2111);

    Entry* e3 = new Entry();
    e3->setNotes("test search term test");
    e3->setGroup(group3);

    Entry* e3b = new Entry();
    e3b->setNotes("test search test");
    e3b->setGroup(group3);

    QList<Entry*> searchResult;

    EntrySearcher entrySearcher;

    searchResult = entrySearcher.search("search term", groupRoot, Qt::CaseInsensitive);

    QCOMPARE(searchResult.count(), 3);

    searchResult = entrySearcher.search("search term", group211, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("search term", group11, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("search term", group1, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 0);
}

void TestEntrySearcher::testAndConcatenationInSearch()
{
    Entry* entry = new Entry();
    entry->setNotes("abc def ghi");
    entry->setTitle("jkl");
    entry->setGroup(groupRoot);

    EntrySearcher entrySearcher;
    QList<Entry*> searchResult;

    searchResult = entrySearcher.search("", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("def", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("  abc    ghi  ", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("ghi ef", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);

    searchResult = entrySearcher.search("abc ef xyz", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 0);

    searchResult = entrySearcher.search("abc kl", groupRoot, Qt::CaseInsensitive);
    QCOMPARE(searchResult.count(), 1);
}
