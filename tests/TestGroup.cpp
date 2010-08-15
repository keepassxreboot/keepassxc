/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "core/Database.h"

class TestGroup : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testParenting();
};

void TestGroup::testParenting()
{
    Database* db = new Database();
    Group* tmpRoot = new Group();

    Group* g1 = new Group();
    Group* g2 = new Group();
    Group* g3 = new Group();
    Group* g4 = new Group();
    g1->setParent(tmpRoot);
    g2->setParent(tmpRoot);
    g3->setParent(tmpRoot);
    g4->setParent(tmpRoot);

    g2->setParent(g1);
    g4->setParent(g3);
    g3->setParent(g1);
    g1->setParent(db);

    QVERIFY(g1->parent() == db);
    QVERIFY(g2->parent() == g1);
    QVERIFY(g3->parent() == g1);
    QVERIFY(g4->parent() == g3);

    QVERIFY(g1->database() == db);
    QVERIFY(g2->database() == db);
    QVERIFY(g3->database() == db);
    QVERIFY(g4->database() == db);

    QVERIFY(tmpRoot->children().size() == 0);
    QVERIFY(g1->children().size() == 2);
    QVERIFY(g2->children().size() == 0);
    QVERIFY(g3->children().size() == 1);
    QVERIFY(g4->children().size() == 0);

    QVERIFY(g1->children().at(0) == g2);
    QVERIFY(g1->children().at(1) == g3);
    QVERIFY(g3->children().contains(g4));

    QSignalSpy spy(db, SIGNAL(groupChanged(const Group*)));
    g2->setName("test");
    g4->setName("test");
    g3->setName("test");
    g1->setName("test");
    g3->setIcon(Uuid::random());
    g1->setIcon(2);
    QVERIFY(spy.count() == 6);
}

QTEST_MAIN(TestGroup);

#include "TestGroup.moc"
