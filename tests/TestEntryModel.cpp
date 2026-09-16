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
#include "core/Metadata.h"
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

void TestEntryModel::testSortByIcon()
{
    auto db = new Database();
    auto root = db->rootGroup();

    auto customIconA = QUuid::fromRfc4122(QByteArray(16, '1'));
    auto customIconB = QUuid::fromRfc4122(QByteArray(16, '2'));
    auto customIconNoName = QUuid::fromRfc4122(QByteArray(16, '3'));
    auto missingIcon = QUuid::fromRfc4122(QByteArray(16, '4'));
    // Referenced by an entry but deliberately never added to the metadata
    // (e.g. leftovers of a deleted or merged custom icon).
    db->metadata()->addCustomIcon(customIconA, QByteArray("icon a"), "Zulu");
    db->metadata()->addCustomIcon(customIconB, QByteArray("icon b"), "Alpha");
    db->metadata()->addCustomIcon(customIconNoName, QByteArray("icon c"));

    auto makeEntry = [&](const QString& title, int iconNumber) {
        auto entry = new Entry();
        entry->setGroup(root);
        entry->setTitle(title);
        entry->setIcon(iconNumber);
        return entry;
    };
    auto makeCustomEntry = [&](const QString& title, const QUuid& uuid) {
        auto entry = new Entry();
        entry->setGroup(root);
        entry->setTitle(title);
        entry->setIcon(uuid);
        return entry;
    };

    auto* icon42 = makeEntry("icon42", 42);
    auto* icon3 = makeEntry("icon3", 3);
    auto* icon7 = makeEntry("icon7", 7);
    auto* namedA = makeCustomEntry("namedA", customIconA);
    auto* namedB = makeCustomEntry("namedB", customIconB);
    auto* unnamed = makeCustomEntry("unnamed", customIconNoName);
    auto* missing = makeCustomEntry("missing", missingIcon);
    auto* icon3Again = makeEntry("icon3again", 3);

    auto modelSource = new EntryModel(this);
    modelSource->setGroup(root);
    // Fix the source order explicitly so that the relative order of entries
    // sharing an icon is deterministic and independent of group iteration
    modelSource->setEntries({icon42, icon3, icon7, namedA, namedB, unnamed, missing, icon3Again});
    QCOMPARE(modelSource->columnCount(), 18);
    QVERIFY(!modelSource->headerData(EntryModel::Icon, Qt::Horizontal, Qt::ToolTipRole).toString().isEmpty());
    // The icon column is icon-only, just like Paperclip and Totp. Qt::DecorationRole
    // is not asserted here because QPixmap cannot be created without a QGuiApplication.
    QVERIFY(modelSource->data(modelSource->index(0, EntryModel::Icon), Qt::DisplayRole).toString().isEmpty());

    auto modelProxy = new SortFilterHideProxyModel(this);
    modelProxy->setSourceModel(modelSource);
    modelProxy->setSortRole(Qt::UserRole);

    auto sortedEntries = [&](Qt::SortOrder order) {
        modelProxy->sort(EntryModel::Icon, order);
        QList<Entry*> entries;
        for (int row = 0; row < modelProxy->rowCount(); ++row) {
            // Map back to the source model: EntryModel::entryFromIndex() takes a
            // source index, while the proxy row order is what the view displays
            entries << modelSource->entryFromIndex(modelProxy->mapToSource(modelProxy->index(row, EntryModel::Icon)));
        }
        return entries;
    };

    // Sort keys, in the order the view shows them, with consecutive duplicates
    // collapsed: entries sharing an icon must be adjacent, i.e. each key must
    // appear exactly once and never be interleaved with another key.
    auto sortedKeys = [&](Qt::SortOrder order) {
        QStringList keys;
        auto sourceRow = [&](Entry* wanted) {
            for (int row = 0; row < modelSource->rowCount(); ++row) {
                if (modelSource->entryFromIndex(modelSource->index(row, 0)) == wanted) {
                    return row;
                }
            }
            return -1;
        };
        for (auto* entry : sortedEntries(order)) {
            auto key =
                modelSource->data(modelSource->index(sourceRow(entry), EntryModel::Icon), Qt::UserRole).toString();
            if (keys.isEmpty() || keys.last() != key) {
                keys << key;
            }
        }
        return keys;
    };

    // Built-in icons (prefixed "0:") are grouped before custom icons (prefixed
    // "1:"). Within a group the keys are compared as strings with a numeric-mode
    // QCollator, which in practice still orders the built-ins lexicographically
    // ("0:42" before "0:7"), and the custom icons by name, case-insensitively.
    // Entries sharing an icon stay adjacent and keep their pre-existing relative
    // order, which the ascending case asserts below.
    QCOMPARE(sortedEntries(Qt::AscendingOrder),
             QList<Entry*>({icon3, icon3Again, icon42, icon7, namedB, namedA, unnamed, missing}));
    QCOMPARE(sortedKeys(Qt::AscendingOrder),
             QStringList({"0:3",
                          "0:42",
                          "0:7",
                          "1:Alpha",
                          "1:Zulu",
                          "1:{33333333-3333-3333-3333-333333333333}",
                          "1:{34343434-3434-3434-3434-343434343434}"}));
    QCOMPARE(sortedKeys(Qt::DescendingOrder),
             QStringList({"1:{34343434-3434-3434-3434-343434343434}",
                          "1:{33333333-3333-3333-3333-333333333333}",
                          "1:Zulu",
                          "1:Alpha",
                          "0:7",
                          "0:42",
                          "0:3"}));
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
