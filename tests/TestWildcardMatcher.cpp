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

#include "TestWildcardMatcher.h"

#include <QTest>

#include "tests.h"
#include "autotype/WildcardMatcher.h"

QTEST_GUILESS_MAIN(TestWildcardMatcher)

const QString TestWildcardMatcher::DefaultText = QString("some text");
const QString TestWildcardMatcher::AlternativeText = QString("some other text");

void TestWildcardMatcher::testMatcher_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("match");

    QTest::newRow("MatchWithoutWildcard") << DefaultText << DefaultText << true;
    QTest::newRow("NoMatchWithoutWildcard") << DefaultText << QString("no match text") << false;
    QTest::newRow("MatchWithOneWildcardInTheMiddle") << AlternativeText << QString("some*text") << true;
    QTest::newRow("NoMatchWithOneWildcardInTheMiddle") << AlternativeText << QString("differen*text") << false;
    QTest::newRow("MatchWithOneWildcardAtBegin") << DefaultText << QString("*text") << true;
    QTest::newRow("NoMatchWithOneWildcardAtBegin") << DefaultText << QString("*text something else") << false;
    QTest::newRow("NatchWithOneWildcardAtEnd") << DefaultText << QString("some*") << true;
    QTest::newRow("NoMatchWithOneWildcardAtEnd") << DefaultText << QString("some other*") << false;
    QTest::newRow("MatchWithMultipleWildcards") << AlternativeText << QString("some*th*text") << true;
    QTest::newRow("NoMatchWithMultipleWildcards") << AlternativeText << QString("some*abc*text") << false;
    QTest::newRow("MatchJustWildcard") << DefaultText << QString("*") << true;
    QTest::newRow("MatchFollowingWildcards") << DefaultText << QString("some t**t") << true;
    QTest::newRow("CaseSensitivity") << DefaultText.toUpper() << QString("some t**t") << true;
}

void TestWildcardMatcher::testMatcher()
{
    QFETCH(QString, text);
    QFETCH(QString, pattern);
    QFETCH(bool, match);

    initMatcher(text);
    verifyMatchResult(pattern, match);
}

void TestWildcardMatcher::initMatcher(QString text)
{
    m_matcher = new WildcardMatcher(text);
}

void TestWildcardMatcher::verifyMatchResult(QString pattern, bool expected)
{
    if (expected) {
        verifyMatch(pattern);
    }
    else {
        verifyNoMatch(pattern);
    }
}

void TestWildcardMatcher::verifyMatch(QString pattern)
{
    bool matchResult = m_matcher->match(pattern);
    QVERIFY(matchResult);
}

void TestWildcardMatcher::verifyNoMatch(QString pattern)
{
    bool matchResult = m_matcher->match(pattern);
    QVERIFY(!matchResult);
}
