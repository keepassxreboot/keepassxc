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
    e3b->setTitle("test search test");
    e3b->setUsername("test@email.com");
    e3b->setPassword("realpass");
    e3b->setGroup(group3);

    // Simple search term testing
    m_searchResult = m_entrySearcher.search("search", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 5);

    m_searchResult = m_entrySearcher.search("search term", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 3);

    m_searchResult = m_entrySearcher.search("search term", group211);
    QCOMPARE(m_searchResult.count(), 1);

    // Test advanced search terms
    m_searchResult = m_entrySearcher.search("password:testpass", m_rootGroup);
    QCOMPARE(m_searchResult.count(), 1);

    m_searchResult = m_entrySearcher.search("!user:email.com", m_rootGroup);
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
    auto terms =
        m_entrySearcher.parseSearchTerms("-test \"quoted \\\"string\\\"\"  user:user pass:\"test me\" noquote  ");

    QCOMPARE(terms.length(), 5);

    QCOMPARE(terms[0]->field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[0]->word, QString("test"));
    QCOMPARE(terms[0]->exclude, true);

    QCOMPARE(terms[1]->field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[1]->word, QString("quoted \\\"string\\\""));
    QCOMPARE(terms[1]->exclude, false);

    QCOMPARE(terms[2]->field, EntrySearcher::Field::Username);
    QCOMPARE(terms[2]->word, QString("user"));

    QCOMPARE(terms[3]->field, EntrySearcher::Field::Password);
    QCOMPARE(terms[3]->word, QString("test me"));

    QCOMPARE(terms[4]->field, EntrySearcher::Field::Undefined);
    QCOMPARE(terms[4]->word, QString("noquote"));

    // Test wildcard and regex search terms
    terms = m_entrySearcher.parseSearchTerms("+url:*.google.com *user:\\d+\\w{2}");

    QCOMPARE(terms.length(), 2);

    QCOMPARE(terms[0]->field, EntrySearcher::Field::Url);
    QCOMPARE(terms[0]->regex.pattern(), QString("^.*\\.google\\.com$"));

    QCOMPARE(terms[1]->field, EntrySearcher::Field::Username);
    QCOMPARE(terms[1]->regex.pattern(), QString("\\d+\\w{2}"));
}
