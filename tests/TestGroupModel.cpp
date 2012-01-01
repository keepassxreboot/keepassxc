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

#include "TestGroupModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modeltest.h"
#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "gui/GroupModel.h"

void TestGroupModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
    Crypto::init();
}

void TestGroupModel::test()
{
    Database* db = new Database();

    Group* groupRoot = db->rootGroup();
    groupRoot->setObjectName("groupRoot");
    groupRoot->setName("groupRoot");

    Group* group1 = new Group();
    group1->setObjectName("group1");
    group1->setName("group1");
    group1->setParent(groupRoot);

    Group* group11 = new Group();
    group1->setObjectName("group11");
    group11->setName("group11");
    group11->setParent(group1);

    Group* group12 = new Group();
    group1->setObjectName("group12");
    group12->setName("group12");
    group12->setParent(group1);

    Group* group121 = new Group();
    group1->setObjectName("group121");
    group121->setName("group121");
    group121->setParent(group12);

    Group* group2 = new Group();
    group1->setObjectName("group2");
    group2->setName("group2");
    group2->setParent(groupRoot);

    GroupModel* model = new GroupModel(db, this);

    ModelTest* modelTest = new ModelTest(model, this);

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

    // test removing a group that has children
    delete group1;

    delete db;

    delete modelTest;
    delete model;
}

KEEPASSX_QTEST_CORE_MAIN(TestGroupModel)
