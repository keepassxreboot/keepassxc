/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTAUTOTYPE_H
#define KEEPASSX_TESTAUTOTYPE_H

#include <QObject>
#include <QSharedPointer>

class AutoType;
class AutoTypePlatformInterface;
class AutoTypeTestInterface;
class Database;
class Entry;
class Group;

class TestAutoType : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testInternal();
    void testSingleAutoType();
    void testGlobalAutoTypeWithNoMatch();
    void testGlobalAutoTypeWithOneMatch();
    void testGlobalAutoTypeTitleMatch();
    void testGlobalAutoTypeUrlMatch();
    void testGlobalAutoTypeUrlSubdomainMatch();
    void testGlobalAutoTypeTitleMatchDisabled();
    void testGlobalAutoTypeRegExp();
    void testAutoTypeResults();
    void testAutoTypeResults_data();
    void testAutoTypeSyntaxChecks();
    void testAutoTypeEffectiveSequences();

private:
    AutoTypePlatformInterface* m_platform;
    AutoTypeTestInterface* m_test;
    AutoType* m_autoType;
    QSharedPointer<Database> m_db;
    QList<QSharedPointer<Database>> m_dbList;
    Group* m_group;
    Entry* m_entry1;
    Entry* m_entry2;
    Entry* m_entry3;
    Entry* m_entry4;
    Entry* m_entry5;
};

#endif // KEEPASSX_TESTAUTOTYPE_H
