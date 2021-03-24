/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTAUTOTYPEEXT_H
#define KEEPASSX_TESTAUTOTYPEEXT_H

#include <QObject>
#include <QSharedPointer>

class AutoType;
class AutoTypeExternalInterface;
class AutoTypeExtTestInterface;
class AutoTypeTargetMap;
class Database;
class Entry;
class Group;

class TestAutoTypeExt : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testExtAutoType();
    void testExtAutoTypeNoTargetSelection();

private:
    AutoTypeExternalInterface* m_platform;
    AutoTypeExtTestInterface* m_test;
    AutoType* m_autoType;
    AutoTypeTargetMap* m_defaultTestMap;

    QSharedPointer<Database> m_db;
    QList<QSharedPointer<Database>> m_dbList;
    Group* m_group;
    Entry* m_entry1;
};

#endif // KEEPASSX_TESTAUTOTYPEEXT_H
