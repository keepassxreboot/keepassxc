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

#include "modeltest.h"
#include "core/Database.h"
#include "core/Group.h"
#include "gui/GroupModel.h"

class TestGroupModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void test();
};

void TestGroupModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void TestGroupModel::test()
{
    Database* db = new Database();

    Group* groupRoot = new Group();
    groupRoot->setParent(db);
    groupRoot->setName("groupRoot");

    Group* group1 = new Group();
    group1->setName("group1");
    group1->setParent(groupRoot);

    Group* group11 = new Group();
    group11->setName("group11");
    group11->setParent(group1);

    Group* group12 = new Group();
    group12->setName("group12");
    group12->setParent(group1);

    Group* group121 = new Group();
    group121->setName("group121");
    group121->setParent(group12);

    Group* group2 = new Group();
    group2->setName("group2");
    group2->setParent(groupRoot);

    GroupModel* model = new GroupModel(db, this);

    new ModelTest(model, this);

    QModelIndex indexRoot = model->index(0, 0);
    QModelIndex index1 = model->index(0, 0, indexRoot);
    QModelIndex index11 = model->index(0, 0, index1);
    QModelIndex index12 = model->index(1, 0, index1);
    QModelIndex index121 = model->index(0, 0, index12);
    QModelIndex index2 = model->index(1, 0, indexRoot);

    QCOMPARE(model->data(indexRoot).toString(), QString("groupRoot"));
    QCOMPARE(model->data(index1).toString(), QString("group1"));
    QCOMPARE(model->data(index11).toString(), QString("group11"));
    QCOMPARE(model->data(index12).toString(), QString("group12"));
    QCOMPARE(model->data(index121).toString(), QString("group121"));
    QCOMPARE(model->data(index2).toString(), QString("group2"));

    QSignalSpy spy1(model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)));
    group11->setName("test");
    group121->setIcon(4);
    QCOMPARE(spy1.count(), 2);

    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex&,int,int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(const QModelIndex&,int,int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&,int,int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)));

    group12->setParent(group1, 0);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    delete groupRoot;
    delete db;
}

QTEST_MAIN(TestGroupModel);

#include "TestGroupModel.moc"
