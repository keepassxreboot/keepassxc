/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include <QSignalSpy>
#include <QTest>

#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseIcons.h"
#include "gui/IconModels.h"
#include "gui/SortFilterHideProxyModel.h"
#include "gui/entry/AutoTypeAssociationsModel.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryModel.h"
#include "modeltest.h"

const auto restoreDefaultLocale = qScopeGuard([prior = QLocale::c()] { QLocale::setDefault(prior); });

QTEST_GUILESS_MAIN(TestEntryModel)

void TestEntryModel::initTestCase()
{
    QLocale::setDefault(QLocale::c());
    qRegisterMetaType<QModelIndex>("QModelIndex");
    QVERIFY(Crypto::init());
}

void TestEntryModel::test()
{
    auto group1 = new Group();
    auto group2 = new Group();

    auto entry1 = new Entry();
    entry1->setGroup(group1);
    entry1->setTitle("testTitle1");

    auto entry2 = new Entry();
    entry2->setGroup(group1);
    entry2->setTitle("testTitle2");

    auto model = new EntryModel(this);

    QSignalSpy spyAboutToBeMoved(model, SIGNAL(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int)));
    QSignalSpy spyMoved(model, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)));

    auto modelTest = new ModelTest(model, this);

    model->setGroup(group1);

    QCOMPARE(model->rowCount(), 2);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    entry1->setTitle("changed");
    QCOMPARE(spyDataChanged.count(), 1);

    QModelIndex index1 = model->index(0, 1);
    QModelIndex index2 = model->index(1, 1);

    QCOMPARE(model->data(index1).toString(), entry1->title());
    QCOMPARE(model->data(index2).toString(), entry2->title());

    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    auto entry3 = new Entry();
    entry3->setGroup(group1);

    QCOMPARE(spyAboutToBeMoved.count(), 0);
    QCOMPARE(spyMoved.count(), 0);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 1);
    QCOMPARE(spyMoved.count(), 1);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    entry3->moveUp();
    QCOMPARE(spyAboutToBeMoved.count(), 3);
    QCOMPARE(spyMoved.count(), 3);

    entry3->moveUp();
    QCOMPARE(spyAboutToBeMoved.count(), 3);
    QCOMPARE(spyMoved.count(), 3);

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

    delete modelTest;
    delete model;
}

void TestEntryModel::testCustomColumns()
{
    Group group;
    auto first = new Entry();
    first->setGroup(&group);
    first->attributes()->set("E-Mail", "b@example.invalid");
    first->attributes()->set("Department", QString::fromUtf8("Büro"));
    auto second = new Entry();
    second->setGroup(&group);
    second->attributes()->set("E-Mail", "a@example.invalid");
    second->attributes()->set("department", "Different attribute");

    EntryModel model;
    model.setCustomColumns({"E-Mail", "Department"});
    ModelTest modelTest(&model, this);
    model.setGroup(&group);
    const int customColumn = EntryModel::ParentGroupPath + 1;
    QCOMPARE(model.columnCount(), customColumn + 2);
    QCOMPARE(model.headerData(customColumn, Qt::Horizontal).toString(), QString("E-Mail"));
    QCOMPARE(model.headerData(customColumn + 1, Qt::Horizontal).toString(), QString("Department"));
    QCOMPARE(model.data(model.index(0, customColumn)).toString(), QString("b@example.invalid"));
    QCOMPARE(model.data(model.index(0, customColumn + 1)).toString(), QString::fromUtf8("Büro"));
    QCOMPARE(model.data(model.index(1, customColumn + 1)).toString(), QString());

    SortFilterHideProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setDynamicSortFilter(true);
    proxy.setSortRole(Qt::UserRole);
    proxy.sort(customColumn);
    QCOMPARE(proxy.mapToSource(proxy.index(0, customColumn)).row(), 1);
    second->attributes()->set("E-Mail", "z@example.invalid");
    QCOMPARE(proxy.mapToSource(proxy.index(0, customColumn)).row(), 0);

    model.setEntries({second, first});
    QCOMPARE(model.data(model.index(0, customColumn)).toString(), QString("z@example.invalid"));
}

void TestEntryModel::testCustomColumnPlaceholders()
{
    Group group;
    auto first = new Entry();
    first->setGroup(&group);
    first->setUsername("b");
    first->attributes()->set("Department", "Sales");
    first->attributes()->set("Contact", "{USERNAME} / {S:Department}");
    auto second = new Entry();
    second->setGroup(&group);
    second->attributes()->set("Contact", "m / Sales");

    EntryModel model;
    model.setCustomColumns({"Contact"});
    model.setGroup(&group);
    const auto index = model.index(0, EntryModel::CustomAttribute);
    QCOMPARE(model.data(index).toString(), QString("b / Sales"));
    QCOMPARE(model.data(index, Qt::UserRole).toString(), QString("b / Sales"));

    SortFilterHideProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setDynamicSortFilter(true);
    proxy.setSortRole(Qt::UserRole);
    proxy.sort(index.column());
    QCOMPARE(proxy.mapToSource(proxy.index(0, index.column())).row(), 0);
    first->setUsername("z");
    QCOMPARE(model.data(index).toString(), QString("z / Sales"));
    QCOMPARE(proxy.mapToSource(proxy.index(0, index.column())).row(), 1);
    first->attributes()->set("Department", "Support");
    QCOMPARE(model.data(index).toString(), QString("z / Support"));
    QCOMPARE(model.data(index, Qt::UserRole).toString(), QString("z / Support"));

    model.setEntries({second, first});
    QCOMPARE(model.data(model.index(1, index.column())).toString(), QString("z / Support"));
}

void TestEntryModel::testCustomColumnProtection()
{
    const auto previousHidePasswords = config()->get(Config::GUI_HidePasswords);
    const auto restoreSettings =
        qScopeGuard([previousHidePasswords] { config()->set(Config::GUI_HidePasswords, previousHidePasswords); });
    config()->set(Config::GUI_HidePasswords, true);
    Group group;
    auto entry = new Entry();
    entry->setGroup(&group);
    entry->setUsername("Test value");
    entry->attributes()->set("Protected", "Test value", true);
    entry->attributes()->set("Empty", "", true);
    entry->attributes()->set("Reference", "{USERNAME}", true);
    entry->attributes()->set("EmptyReference", "{S:Missing}", true);
    EntryModel model;
    model.setCustomColumns({"Protected", "Empty", "Reference", "EmptyReference"});
    model.setGroup(&group);
    const auto index = model.index(0, EntryModel::ParentGroupPath + 1);
    QCOMPARE(model.data(index).toString(), QString("\u25cf").repeated(6));
    QCOMPARE(model.data(index, Qt::UserRole).toString(), QString("Test value"));
    QVERIFY(!model.data(index, Qt::ToolTipRole).isValid());
    QCOMPARE(model.data(model.index(0, index.column() + 1)).toString(), QString());
    const auto referenceIndex = model.index(0, index.column() + 2);
    QCOMPARE(model.data(referenceIndex).toString(), QString("\u25cf").repeated(6));
    QCOMPARE(model.data(referenceIndex, Qt::UserRole).toString(), QString("Test value"));
    QVERIFY(!model.data(referenceIndex, Qt::ToolTipRole).isValid());
    QCOMPARE(model.data(model.index(0, index.column() + 3)).toString(), QString());
    QSignalSpy changed(&model, &EntryModel::dataChanged);
    config()->set(Config::GUI_HidePasswords, false);
    QVERIFY(!changed.isEmpty());
    QCOMPARE(model.data(index).toString(), QString("Test value"));
    QCOMPARE(model.data(referenceIndex).toString(), QString("Test value"));
}

void TestEntryModel::testCustomColumnChanges()
{
    Group group;
    auto entry = new Entry();
    entry->setGroup(&group);
    EntryModel model;
    ModelTest modelTest(&model, this);
    model.setGroup(&group);
    QPersistentModelIndex title(model.index(0, EntryModel::Title));
    QSignalSpy inserted(&model, &EntryModel::columnsInserted);
    QSignalSpy removed(&model, &EntryModel::columnsRemoved);
    model.setCustomColumns({"E-Mail"});
    QCOMPARE(inserted.count(), 1);
    model.setCustomColumns({"E-Mail", "Department"});
    QCOMPARE(inserted.count(), 2);
    QCOMPARE(removed.count(), 0);
    QVERIFY(title.isValid());
    QCOMPARE(title.column(), int(EntryModel::Title));
    model.setCustomColumns({"E-Mail", "Phone"});
    QCOMPARE(removed.count(), 1);
    QCOMPARE(model.headerData(EntryModel::ParentGroupPath + 2, Qt::Horizontal).toString(), QString("Phone"));
    model.setCustomColumns({});
    QCOMPARE(model.columnCount(), int(EntryModel::ParentGroupPath + 1));
    QVERIFY(title.isValid());
    model.setCustomColumns({"", "Password", "Title", "E-Mail", "E-Mail"});
    QCOMPARE(model.customColumns(), QStringList({"E-Mail"}));
    model.setCustomColumns({"E-Mail", "Department", "Phone"});
    QPersistentModelIndex phone(model.index(0, EntryModel::CustomAttribute + 2));
    model.setCustomColumns({"E-Mail", "Phone"});
    QVERIFY(phone.isValid());
    QCOMPARE(phone.column(), int(EntryModel::CustomAttribute + 1));
    QCOMPARE(model.headerData(phone.column(), Qt::Horizontal).toString(), QString("Phone"));
}

void TestEntryModel::testAttachmentsModel()
{
    auto entryAttachments = new EntryAttachments(this);

    auto model = new EntryAttachmentsModel(this);
    auto modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);
    model->setEntryAttachments(entryAttachments);
    QCOMPARE(model->rowCount(), 0);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    entryAttachments->set("first", QByteArray("123"));

    entryAttachments->set("2nd", QByteArray("456"));
    entryAttachments->set("2nd", QByteArray("7890"));

    const int firstRow = 0;
    QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::NameColumn)).toString(), QString("2nd"));
    QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::SizeColumn), Qt::EditRole).toInt(), 4);

    entryAttachments->remove("first");

    QCOMPARE(spyDataChanged.count(), 1);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    entryAttachments->clear();
    model->setEntryAttachments(nullptr);
    QCOMPARE(spyReset.count(), 2);
    QCOMPARE(model->rowCount(), 0);

    delete modelTest;
    delete model;
    delete entryAttachments;
}

void TestEntryModel::testAttributesModel()
{
    auto entryAttributes = new EntryAttributes(this);

    auto model = new EntryAttributesModel(this);
    auto modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);
    model->setEntryAttributes(entryAttributes);
    QCOMPARE(model->rowCount(), 0);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    entryAttributes->set("first", "123");

    entryAttributes->set("2nd", "456");
    entryAttributes->set("2nd", "789");

    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("2nd"));

    entryAttributes->remove("first");

    // make sure these don't generate messages
    entryAttributes->set("Title", "test");
    entryAttributes->set("UserName", "test");
    entryAttributes->set("Password", "test");
    entryAttributes->set("URL", "test");
    entryAttributes->set("Notes", "test");

    QCOMPARE(spyDataChanged.count(), 1);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    // test attribute protection
    QString value = entryAttributes->value("2nd");
    entryAttributes->set("2nd", value, true);
    QVERIFY(entryAttributes->isProtected("2nd"));
    QCOMPARE(entryAttributes->value("2nd"), value);
    entryAttributes->clear();

    // test attribute sorting (ASCII)
    entryAttributes->set("Test1", "1");
    entryAttributes->set("Test2", "2");
    entryAttributes->set("Test11", "11");
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("Test1"));
    QCOMPARE(model->data(model->index(1, 0)).toString(), QString("Test11"));
    QCOMPARE(model->data(model->index(2, 0)).toString(), QString("Test2"));

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    entryAttributes->clear();
    model->setEntryAttributes(nullptr);
    QCOMPARE(spyReset.count(), 2);
    QCOMPARE(model->rowCount(), 0);

    delete modelTest;
    delete model;
}

void TestEntryModel::testDefaultIconModel()
{
    auto model = new DefaultIconModel(this);
    auto modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), databaseIcons()->count());

    delete modelTest;
    delete model;
}

void TestEntryModel::testCustomIconModel()
{
    auto model = new CustomIconModel(this);
    auto modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);

    QHash<QUuid, QPixmap> icons;
    QList<QUuid> iconsOrder;

    QUuid iconUuid = QUuid::fromRfc4122(QByteArray(16, '2'));
    icons.insert(iconUuid, QPixmap());
    iconsOrder << iconUuid;

    QUuid iconUuid2 = QUuid::fromRfc4122(QByteArray(16, '1'));
    icons.insert(iconUuid2, QPixmap());
    iconsOrder << iconUuid2;

    model->setIcons(icons, iconsOrder);
    QCOMPARE(model->uuidFromIndex(model->index(0, 0)), iconUuid);
    QCOMPARE(model->uuidFromIndex(model->index(1, 0)), iconUuid2);

    delete modelTest;
    delete model;
}

void TestEntryModel::testAutoTypeAssociationsModel()
{
    auto model = new AutoTypeAssociationsModel(this);
    auto modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);

    auto associations = new AutoTypeAssociations(this);
    model->setAutoTypeAssociations(associations);

    QCOMPARE(model->rowCount(), 0);

    AutoTypeAssociations::Association assoc;
    assoc.window = "1";
    assoc.sequence = "2";
    associations->add(assoc);

    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("1"));
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("2"));

    assoc.window = "3";
    assoc.sequence = "4";
    associations->update(0, assoc);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("3"));
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("4"));

    associations->add(assoc);
    associations->remove(0);
    QCOMPARE(model->rowCount(), 1);

    delete modelTest;
    delete model;
    delete associations;
}

void TestEntryModel::testProxyModel()
{
    auto modelSource = new EntryModel(this);
    auto modelProxy = new SortFilterHideProxyModel(this);
    modelProxy->setSourceModel(modelSource);

    auto modelTest = new ModelTest(modelProxy, this);

    auto db = new Database();
    auto entry = new Entry();
    entry->setTitle("Test Title");
    entry->setGroup(db->rootGroup());

    modelSource->setGroup(db->rootGroup());

    // Test hiding and showing a column
    auto columnCount = modelProxy->columnCount();
    QSignalSpy spyColumnRemove(modelProxy, SIGNAL(columnsAboutToBeRemoved(QModelIndex, int, int)));
    modelProxy->hideColumn(0, true);
    QCOMPARE(modelProxy->columnCount(), columnCount - 1);
    QVERIFY(!spyColumnRemove.isEmpty());

    int oldSpyColumnRemoveSize = spyColumnRemove.size();
    modelProxy->hideColumn(0, true);
    QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

    modelProxy->hideColumn(100, true);
    QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

    QList<Entry*> entryList;
    entryList << entry;
    modelSource->setEntries(entryList);

    QSignalSpy spyColumnInsert(modelProxy, SIGNAL(columnsAboutToBeInserted(QModelIndex, int, int)));
    modelProxy->hideColumn(0, false);
    QCOMPARE(modelProxy->columnCount(), columnCount);
    QVERIFY(!spyColumnInsert.isEmpty());

    int oldSpyColumnInsertSize = spyColumnInsert.size();
    modelProxy->hideColumn(0, false);
    QCOMPARE(spyColumnInsert.size(), oldSpyColumnInsertSize);

    delete modelTest;
    delete modelProxy;
    delete modelSource;
    delete db;
}

void TestEntryModel::testDatabaseDelete()
{
    auto model = new EntryModel(this);
    auto modelTest = new ModelTest(model, this);

    auto db1 = new Database();
    auto group1 = new Group();
    group1->setParent(db1->rootGroup());

    auto entry1 = new Entry();
    entry1->setGroup(group1);

    auto db2 = new Database();
    auto entry2 = new Entry();
    entry2->setGroup(db2->rootGroup());

    model->setEntries(QList<Entry*>() << entry1 << entry2);

    QCOMPARE(model->rowCount(), 2);

    delete db1;
    QCOMPARE(model->rowCount(), 1);

    delete entry2;
    QCOMPARE(model->rowCount(), 0);

    delete db2;
    delete modelTest;
    delete model;
}
