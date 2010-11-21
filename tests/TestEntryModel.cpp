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

#include "TestEntryModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modeltest.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "gui/EntryModel.h"

void TestEntryModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void TestEntryModel::test()
{
    Group* group1 = new Group();
    Group* group2 = new Group();

    Entry* entry1 = new Entry();
    entry1->setGroup(group1);
    entry1->setTitle("testTitle1");

    Entry* entry2 = new Entry();
    entry2->setGroup(group1);
    entry2->setTitle("testTitle2");

    EntryModel* model = new EntryModel(this);

    new ModelTest(model, this);

    model->setGroup(group1);

    QCOMPARE(model->rowCount(), 2);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)));
    entry1->setTitle("changed");
    QCOMPARE(spyDataChanged.count(), 1);

    QModelIndex index1 = model->index(0, 0);
    QModelIndex index2 = model->index(1, 0);

    QCOMPARE(model->data(index1).toString(), entry1->title());
    QCOMPARE(model->data(index2).toString(), entry2->title());

    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex&,int,int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(const QModelIndex&,int,int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&,int,int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)));

    Entry* entry3 = new Entry();
    entry3->setGroup(group1);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);

    entry2->setGroup(group2);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    model->setGroup(group2);
    QCOMPARE(spyReset.count(), 1);

    delete group1;
    delete group2;
}

QTEST_MAIN(TestEntryModel);
