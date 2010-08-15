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

#include <QtTest/QTest>

#include "modeltest.h"
#include "core/Group.h"
#include "gui/GroupModel.h"

class TestGroupModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test();
};

void TestGroupModel::test()
{
    Group* groupRoot = new Group();
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

    GroupModel* model = new GroupModel(groupRoot, this);

    new ModelTest(model, this);

    QModelIndex indexRoot = model->index(0, 0);
    QModelIndex index1 = model->index(0, 0, indexRoot);
    QModelIndex index11 = model->index(0, 0, index1);
    QModelIndex index12 = model->index(1, 0, index1);
    QModelIndex index121 = model->index(0, 0, index12);
    QModelIndex index2 = model->index(1, 0, indexRoot);

    QVERIFY(model->data(indexRoot) == "groupRoot");
    QVERIFY(model->data(index1) == "group1");
    QVERIFY(model->data(index11) == "group11");
    QVERIFY(model->data(index12) == "group12");
    QVERIFY(model->data(index121) == "group121");
    QVERIFY(model->data(index2) == "group2");

    delete groupRoot;
}

QTEST_MAIN(TestGroupModel);

#include "TestGroupModel.moc"
