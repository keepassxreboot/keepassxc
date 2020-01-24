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
#include "TestGlobal.h"

QTEST_GUILESS_MAIN(TestEntrySearcher)

void TestEntrySearcher::init()
{
    m_rootGroup = new Group();
    m_entrySearcher = EntrySearcher();
}

void TestEntrySearcher::cleanup()
{
    delete m_rootGroup;
}

void TestEntrySearcher::testSearch()
{
    /**
     * Root
     * - group1 (search disabled)
     *   - group11
     * - group2
     *   - group21
     *     - group211
     *       - group2111
     */
    Group* group1 = new Group();
    Group* group2 = new Group();
    Group* group3 = new Group();

    group1->setParent(m_rootGroup);
    group2->setParent(m_rootGroup);
    group3->setParent(m_rootGroup);

    Group* group11 = new Group();

    group11->setParent(group1);

    Group* group21 = new Group();
    Group* group211 = new Group();
    Group* group2111 = new Group();

    group21->setParent(group2);
    group211->setParent(group21);
    group2111->setParent(group211);

    group1->setSearchingEnabled(Group::Disable);

    Entry* eRoot = new Entry();
    eRoot->setTitle("test search term test");
    eRoot->setGroup(m_rootGroup);

    Entry* eRoot2 = new Entry();
    eRoot2->setNotes("test term test");
    eRoot2->setGroup(m_rootGroup);

    // Searching is disabled for these
    Entry* e1 = new Entry();
    e1->setUsername("test search term test");
    e1->setGroup(group1);

    Entry* e11 = new Entry();
    e11->setNotes("test search term test");
    e11->setGroup(group11);
    // End searching disabled

    Entry* e2111 = new Entry();
    e2111->setTitle("test search term test");
    e2111->setGroup(group2111);

    Entry* e2111b = new Entry();
    e2111b->setNotes("test search test");
    e2111b->setUsername("user123");
    e2111b->setPassword("testpass");
    e2111b->setGroup(group2111);

    Entry* e3 = new Entry();
    e3->setUrl("test search term test");
    e3->setGroup(group3);

    Entry* e3b = new Entry();
    e3b->setTitle("test search test 123");
    e3b->setUsername("test@email.com");
    e3b->setPassword("realpass");
    e3b->setGroup(group3);

    // Simple search term testing
    m_searchResult = m_entrySearcher.search("search", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 5);

    m_searchResult = m_entrySearcher.search("search term", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 3);

    m_searchResult = m_entrySearcher.search("123", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 2);

    m_searchResult = m_entrySearcher.search("search term", group211);
    QCOMPARE(m_searchResult.count(), 1);

    // Test advanced search terms
    m_searchResult = m_entrySearcher.search("title:123", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("t:123", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("password:testpass", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("pw:testpass", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("!user:email.com", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 5);

    m_searchResult = m_entrySearcher.search("!u:email.com", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 5);

    m_searchResult = m_entrySearcher.search("*user:\".*@.*\\.com\"", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("+user:email", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 0);

    // Terms are logical AND together
    m_searchResult = m_entrySearcher.search("password:pass user:user", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    // Parent group has search disabled
    m_searchResult = m_entrySearcher.search("search term", group11);
    QCOMPARE(m_searchResult.count(), 0);
}

void TestEntrySearcher::testAndConcatenationInSearch()
{
    Entry* entry = new Entry();
    entry->setNotes("abc def ghi");
    entry->setTitle("jkl");
    entry->setGroup(m_rootGroup);

    m_searchResult = m_entrySearcher.search("", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("def", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("  abc    ghi  ", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("ghi ef", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("abc ef xyz", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 0);

    m_searchResult = m_entrySearcher.search("abc kl", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testAllAttributesAreSearched()
{
    Entry* entry = new Entry();
    entry->setGroup(m_rootGroup);

    entry->setTitle("testTitle");
    entry->setUsername("testUsername");
    entry->setUrl("testUrl");
    entry->setNotes("testNote");

    // Default is to AND all terms together
    m_searchResult = m_entrySearcher.search("testTitle testUsername testUrl testNote", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testSearchTermParser()
{
    // Test standard search terms
    m_entrySearcher.parseSearchTerms("-test \"quoted \\\"string\\\"\"  user:user pass:\"test me\" noquote  ");
    auto terms = m_entrySearcher.m_searchTerms;

    QCOMPARE(terms.length(), 5);

    QCOMPARE(terms[0].field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[0].word, QString("test"));
    QCOMPARE(terms[0].exclude, true);

    QCOMPARE(terms[1].field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[1].word, QString("quoted \\\"string\\\""));
    QCOMPARE(terms[1].exclude, false);

    QCOMPARE(terms[2].field, EntrySearcher::Field::Username);
    QCOMPARE(terms[2].word, QString("user"));

    QCOMPARE(terms[3].field, EntrySearcher::Field::Password);
    QCOMPARE(terms[3].word, QString("test me"));

    QCOMPARE(terms[4].field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[4].word, QString("noquote"));

    // Test wildcard and regex search terms
    m_entrySearcher.parseSearchTerms("+url:*.google.com *user:\\d+\\w{2}");
    terms = m_entrySearcher.m_searchTerms;

    QCOMPARE(terms.length(), 2);

    QCOMPARE(terms[0].field, EntrySearcher::Field::Url);
    QCOMPARE(terms[0].regex.pattern(), QString("^.*\\.google\\.com$"));

    QCOMPARE(terms[1].field, EntrySearcher::Field::Username);
    QCOMPARE(terms[1].regex.pattern(), QString("\\d+\\w{2}"));

    // Test custom attribute search terms
    m_entrySearcher.parseSearchTerms("+_abc:efg _def:\"ddd\"");
    terms = m_entrySearcher.m_searchTerms;

    QCOMPARE(terms.length(), 2);

    QCOMPARE(terms[0].field, EntrySearcher::Field::AttributeValue);
    QCOMPARE(terms[0].word, QString("abc"));
    QCOMPARE(terms[0].regex.pattern(), QString("^efg$"));

    QCOMPARE(terms[1].field, EntrySearcher::Field::AttributeValue);
    QCOMPARE(terms[1].word, QString("def"));
    QCOMPARE(terms[1].regex.pattern(), QString("ddd"));
}

void TestEntrySearcher::testCustomAttributesAreSearched()
{
    QScopedPointer<Entry> e1(new Entry());
    e1->setGroup(m_rootGroup);

    e1->attributes()->set("testAttribute", "testE1");
    e1->attributes()->set("testProtected", "testP", true);

    QScopedPointer<Entry> e2(new Entry());
    e2->setGroup(m_rootGroup);
    e2->attributes()->set("testAttribute", "testE2");
    e2->attributes()->set("testProtected", "testP2", true);

    // search for custom entries
    m_searchResult = m_entrySearcher.search("_testAttribute:test", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 2);

    // protected attributes are ignored
    m_entrySearcher = EntrySearcher(false, true);
    m_searchResult = m_entrySearcher.search("_testAttribute:test _testProtected:testP2", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 2);
}

void TestEntrySearcher::testGroup()
{
    /**
     * Root
     * - group1 (1 entry)
     *   - subgroup1 (2 entries)
     * - group2
     *   - subgroup2 (1 entry)
     */
    Group* group1 = new Group();
    Group* group2 = new Group();

    group1->setParent(m_rootGroup);
    group1->setName("group1");
    group2->setParent(m_rootGroup);
    group2->setName("group2");

    Group* subgroup1 = new Group();
    subgroup1->setName("subgroup1");
    subgroup1->setParent(group1);

    Group* subgroup2 = new Group();
    subgroup2->setName("subgroup2");
    subgroup2->setParent(group2);

    Entry* eGroup1 = new Entry();
    eGroup1->setTitle("Entry Group 1");
    eGroup1->setGroup(group1);

    Entry* eSub1 = new Entry();
    eSub1->setTitle("test search term test");
    eSub1->setGroup(subgroup1);

    Entry* eSub2 = new Entry();
    eSub2->setNotes("test test");
    eSub2->setGroup(subgroup1);

    Entry* eSub3 = new Entry();
    eSub3->setNotes("test term test");
    eSub3->setGroup(subgroup2);

    m_searchResult = m_entrySearcher.search("group:subgroup", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 3);

    m_searchResult = m_entrySearcher.search("g:subgroup1", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 2);

    m_searchResult = m_entrySearcher.search("g:subgroup1 search", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("g:*1/sub*1", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 2);

    m_searchResult = m_entrySearcher.search("g:/group1 search", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);
}

void TestEntrySearcher::testSkipProtected()
{
    QScopedPointer<Entry> e1(new Entry());
    e1->setGroup(m_rootGroup);

    e1->attributes()->set("testAttribute", "testE1");
    e1->attributes()->set("testProtected", "apple", true);

    QScopedPointer<Entry> e2(new Entry());
    e2->setGroup(m_rootGroup);
    e2->attributes()->set("testAttribute", "testE2");
    e2->attributes()->set("testProtected", "banana", true);

    const QList<Entry*> expectE1{e1.data()};
    const QList<Entry*> expectE2{e2.data()};
    const QList<Entry*> expectBoth{e1.data(), e2.data()};

    // when not skipping protected, empty term matches everything
    m_searchResult = m_entrySearcher.search("", m_rootGroup);
    QCOMPARE(m_searchResult, expectBoth);

    // now test the searcher with skipProtected = true
    m_entrySearcher = EntrySearcher(false, true);

    // when skipping protected, empty term matches nothing
    m_searchResult = m_entrySearcher.search("", m_rootGroup);
    QCOMPARE(m_searchResult, {});

    // having a protected entry in terms should not affect the results in anyways
    m_searchResult = m_entrySearcher.search("_testProtected:apple", m_rootGroup);
    QCOMPARE(m_searchResult, {});
    m_searchResult = m_entrySearcher.search("_testProtected:apple _testAttribute:testE2", m_rootGroup);
    QCOMPARE(m_searchResult, expectE2);
    m_searchResult = m_entrySearcher.search("_testProtected:apple _testAttribute:testE1", m_rootGroup);
    QCOMPARE(m_searchResult, expectE1);
    m_searchResult =
        m_entrySearcher.search("_testProtected:apple _testAttribute:testE1 _testAttribute:testE2", m_rootGroup);
    QCOMPARE(m_searchResult, {});

    // also move the protected term around to execurise the short-circut logic
    m_searchResult = m_entrySearcher.search("_testAttribute:testE2 _testProtected:apple", m_rootGroup);
    QCOMPARE(m_searchResult, expectE2);
    m_searchResult = m_entrySearcher.search("_testAttribute:testE1 _testProtected:apple", m_rootGroup);
    QCOMPARE(m_searchResult, expectE1);
    m_searchResult =
        m_entrySearcher.search("_testAttribute:testE1 _testProtected:apple _testAttribute:testE2", m_rootGroup);
    QCOMPARE(m_searchResult, {});
}
