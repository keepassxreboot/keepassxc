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

#ifndef KEEPASSX_TESTWILDCARDMATCHER_H
#define KEEPASSX_TESTWILDCARDMATCHER_H

#include <QtCore/QObject>

class WildcardMatcher;

class TestWildcardMatcher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testMatchWithoutWildcard();
    void testNoMatchWithoutWildcard();
    void testMatchWithOneWildcardInTheMiddle();
    void testNoMatchWithOneWildcardInTheMiddle();
    void testNoMatchWithOneWildcardAtBegin();
    void testMatchWithOneWildcardAtBegin();
    void testNoMatchWithOneWildcardAtEnd();
    void testMatchWithOneWildcardAtEnd();
    void testMatchWithMultipleWildcards();
    void testNoMatchWithMultipleWildcards();
    void testMatchJustWildcard();
    void testMatchFollowingWildcards();
    void testCaseSensitivity();

private:
    static const QString DefaultText;
    static const QString AlternativeText;

    void initMatcher(QString text);
    void verifyMatch(QString pattern);
    void verifyNoMatch(QString pattern);

    WildcardMatcher* m_matcher;
};

#endif // KEEPASSX_TESTWILDCARDMATCHER_H
