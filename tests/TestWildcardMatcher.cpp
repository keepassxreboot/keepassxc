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

#include <QtTest/QTest>

#include "tests.h"
#include "autotype/WildcardMatcher.h"

const QString TestWildcardMatcher::DefaultText = QString("some text");
const QString TestWildcardMatcher::AlternativeText = QString("some other text");

void TestWildcardMatcher::testMatchWithoutWildcard()
{
    initMatcher(DefaultText);
    QString pattern = DefaultText;
    verifyMatch(pattern);
}

void TestWildcardMatcher::testNoMatchWithoutWildcard()
{
    initMatcher(DefaultText);
    QString pattern = QString("no match text");
    verifyNoMatch(pattern);
}

void TestWildcardMatcher::testMatchWithOneWildcardInTheMiddle()
{
    initMatcher(AlternativeText);
    QString pattern("some*text");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testNoMatchWithOneWildcardInTheMiddle()
{
    initMatcher(AlternativeText);
    QString pattern("differen*text");
    verifyNoMatch(pattern);
}

void TestWildcardMatcher::testMatchWithOneWildcardAtBegin()
{
    initMatcher(DefaultText);
    QString pattern("*text");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testNoMatchWithOneWildcardAtBegin()
{
    initMatcher(DefaultText);
    QString pattern("*text something else");
    verifyNoMatch(pattern);
}

void TestWildcardMatcher::testMatchWithOneWildcardAtEnd()
{
    initMatcher(DefaultText);
    QString pattern("some*");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testNoMatchWithOneWildcardAtEnd()
{
    initMatcher(DefaultText);
    QString pattern("some other*");
    verifyNoMatch(pattern);
}

void TestWildcardMatcher::testMatchWithMultipleWildcards()
{
    initMatcher(AlternativeText);
    QString pattern("some*th*text");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testNoMatchWithMultipleWildcards()
{
    initMatcher(AlternativeText);
    QString pattern("some*abc*text");
    verifyNoMatch(pattern);
}

void TestWildcardMatcher::testMatchJustWildcard()
{
    initMatcher(AlternativeText);
    QString pattern("*");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testMatchFollowingWildcards()
{
    initMatcher(DefaultText);
    QString pattern("some t**t");
    verifyMatch(pattern);
}

void TestWildcardMatcher::testCaseSensitivity()
{
    initMatcher(DefaultText.toUpper());
    QString pattern("some t**t");
    verifyMatch(pattern);
}

void TestWildcardMatcher::initMatcher(QString text)
{
    m_matcher = new WildcardMatcher(text);
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

KEEPASSX_QTEST_CORE_MAIN(TestWildcardMatcher)
