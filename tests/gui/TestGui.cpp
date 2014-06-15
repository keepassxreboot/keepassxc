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

#include "TestGui.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTest>
#include <QToolBar>
#include <QToolButton>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "gui/group/GroupModel.h"
#include "gui/group/GroupView.h"
#include "keys/PasswordKey.h"

void TestGui::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    m_mainWindow = new MainWindow();
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();
    Tools::wait(50);
}

void TestGui::testOpenDatabase()
{
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    triggerAction("actionDatabaseOpen");

    QWidget* databaseOpenWidget = m_mainWindow->findChild<QWidget*>("databaseOpenWidget");
    QLineEdit* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);
}

void TestGui::testTabs()
{
    QCOMPARE(m_tabWidget->count(), 1);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("NewDatabase.kdbx"));

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

void TestGui::testEditEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QModelIndex item = entryView->model()->index(0, 1);
    QRect itemRect = entryView->visualRect(item);
    QTest::mouseClick(entryView->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QVERIFY(m_dbWidget->currentWidget() == editEntryWidget);
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    // make sure the database isn't marked as modified
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("NewDatabase.kdbx"));
}

void TestGui::testAddEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    QTest::mouseClick(entryNewWidget, Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->historyItems().size(), 0);
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("NewDatabase.kdbx*"));

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    QTest::keyClicks(titleEdit, "something");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(entry->title(), QString("testsomething"));
    QCOMPARE(entry->historyItems().size(), 1);


    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 2");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);


    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 3");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);


    QTRY_COMPARE(entryView->model()->rowCount(), 4);
}

void TestGui::testSearch()
{
    QAction* searchAction = m_mainWindow->findChild<QAction*>("actionSearch");
    QVERIFY(searchAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* searchActionWidget = toolBar->widgetForAction(searchAction);
    QVERIFY(searchActionWidget->isEnabled());
    QTest::mouseClick(searchActionWidget, Qt::LeftButton);

    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QLineEdit* searchEdit = m_dbWidget->findChild<QLineEdit*>("searchEdit");
    QToolButton* clearSearch = m_dbWidget->findChild<QToolButton*>("clearButton");

    QTest::keyClicks(searchEdit, "ZZZ");

    QTRY_COMPARE(entryView->model()->rowCount(), 0);

    QTest::mouseClick(clearSearch, Qt::LeftButton);
    QTest::keyClicks(searchEdit, "some");

    QTRY_COMPARE(entryView->model()->rowCount(), 4);

    clickIndex(entryView->model()->index(0, 1), entryView, Qt::LeftButton);
    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);

    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton);
    QAction* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");

    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);
    QVERIFY(entryDeleteWidget->isVisible());
    QVERIFY(entryDeleteWidget->isEnabled());
    QVERIFY(!m_db->metadata()->recycleBin());

    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(2, 0), entryView, Qt::LeftButton, Qt::ControlModifier);
    QCOMPARE(entryView->selectionModel()->selectedRows().size(), 2);

    MessageBox::setNextAnswer(QMessageBox::No);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 1);

    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 1);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    QWidget* closeSearchButton = m_dbWidget->findChild<QToolButton*>("closeSearchButton");
    QTest::mouseClick(closeSearchButton, Qt::LeftButton);

    QCOMPARE(entryView->model()->rowCount(), 1);
}

void TestGui::testDeleteEntry()
{
    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QAction* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");
    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());

    QModelIndex rootGroupIndex = groupView->model()->index(0, 0);
    clickIndex(groupView->model()->index(groupView->model()->rowCount(rootGroupIndex) - 1, 0, rootGroupIndex),
               groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup()->name(), m_db->metadata()->recycleBin()->name());

    clickIndex(entryView->model()->index(0, 0), entryView, Qt::LeftButton);
    MessageBox::setNextAnswer(QMessageBox::No);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 3);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 3);

    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 2);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 2);

    clickIndex(entryView->model()->index(0, 0), entryView, Qt::LeftButton);
    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton, Qt::ControlModifier);
    MessageBox::setNextAnswer(QMessageBox::Yes);
    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QCOMPARE(entryView->model()->rowCount(), 0);
    QCOMPARE(m_db->metadata()->recycleBin()->entries().size(), 0);

    clickIndex(groupView->model()->index(0, 0),
               groupView, Qt::LeftButton);
    QCOMPARE(groupView->currentGroup(), m_db->rootGroup());
}

void TestGui::testCloneEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    QCOMPARE(entryView->model()->rowCount(), 1);

    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entryOrg = entryView->entryFromIndex(item);
    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntryClone");

    QCOMPARE(entryView->model()->rowCount(), 2);
    Entry* entryClone = entryView->entryFromIndex(entryView->model()->index(1, 1));
    QVERIFY(entryOrg->uuid() != entryClone->uuid());
    QCOMPARE(entryClone->title(), entryOrg->title());
}

void TestGui::testDragAndDropEntry()
{
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    QAbstractItemModel* groupModel = groupView->model();

    QModelIndex sourceIndex = entryView->model()->index(0, 1);
    QModelIndex targetIndex = groupModel->index(0, 0, groupModel->index(0, 0));
    QVERIFY(sourceIndex.isValid());
    QVERIFY(targetIndex.isValid());

    QMimeData mimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    Entry* entry = entryView->entryFromIndex(sourceIndex);
    stream << entry->group()->database()->uuid() << entry->uuid();
    mimeData.setData("application/x-keepassx-entry", encoded);

    QVERIFY(groupModel->dropMimeData(&mimeData, Qt::MoveAction, -1, 0, targetIndex));
    QCOMPARE(entry->group()->name(), QString("General"));
}

void TestGui::testDragAndDropGroup()
{
    QAbstractItemModel* groupModel = m_dbWidget->findChild<GroupView*>("groupView")->model();
    QModelIndex rootIndex = groupModel->index(0, 0);

    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     groupModel->index(1, 0, rootIndex),
                     -1, true, "Windows", 0);

    // dropping parent on child is supposed to fail
    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     groupModel->index(0, 0, groupModel->index(0, 0, rootIndex)),
                     -1, false, "NewDatabase", 0);

    dragAndDropGroup(groupModel->index(1, 0, rootIndex),
                     rootIndex,
                     0, true, "NewDatabase", 0);

    dragAndDropGroup(groupModel->index(0, 0, rootIndex),
                     rootIndex,
                     -1, true, "NewDatabase", 5);
}

void TestGui::testSaveAs()
{
    QFileInfo fileInfo(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QDateTime lastModified = fileInfo.lastModified();

    m_db->metadata()->setName("SaveAs");

    QTemporaryFile* tmpFile = new QTemporaryFile();
    // open temporary file so it creates a filename
    QVERIFY(tmpFile->open());
    m_tmpFileName = tmpFile->fileName();
    delete tmpFile;
    fileDialog()->setNextFileName(m_tmpFileName);

    triggerAction("actionDatabaseSaveAs");

    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("SaveAs"));

    checkDatabase();

    fileInfo.refresh();
    QCOMPARE(fileInfo.lastModified(), lastModified);
}

void TestGui::testSave()
{
    m_db->metadata()->setName("Save");
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    checkDatabase();
}

void TestGui::testDatabaseSettings()
{
    triggerAction("actionChangeDatabaseSettings");
    QWidget* dbSettingsWidget = m_dbWidget->findChild<QWidget*>("databaseSettingsWidget");
    QSpinBox* transformRoundsSpinBox = dbSettingsWidget->findChild<QSpinBox*>("transformRoundsSpinBox");
    transformRoundsSpinBox->setValue(100);
    QTest::keyClick(transformRoundsSpinBox, Qt::Key_Enter);
    // wait for modified timer
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));
    QCOMPARE(m_db->transformRounds(), Q_UINT64_C(100));

    triggerAction("actionDatabaseSave");
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    checkDatabase();
}

void TestGui::testKeePass1Import()
{
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb"));
    triggerAction("actionImportKeePass1");

    QWidget* keepass1OpenWidget = m_mainWindow->findChild<QWidget*>("keepass1OpenWidget");
    QLineEdit* editPassword = keepass1OpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "masterpw");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QCOMPARE(m_tabWidget->count(), 2);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("basic [New database]*"));
}

void TestGui::testDatabaseLocking()
{
    triggerAction("actionLockDatabases");

    QCOMPARE(m_tabWidget->tabText(0), QString("Save [locked]"));
    QCOMPARE(m_tabWidget->tabText(1), QString("basic [New database] [locked]*"));

    QWidget* dbWidget = m_tabWidget->currentDatabaseWidget();
    QWidget* unlockDatabaseWidget = dbWidget->findChild<QWidget*>("unlockDatabaseWidget");
    QWidget* editPassword = unlockDatabaseWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "masterpw");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("basic [New database]*"));
}

void TestGui::cleanupTestCase()
{
    delete m_mainWindow;
    QFile::remove(m_tmpFileName);
}

void TestGui::checkDatabase()
{
    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    QScopedPointer<Database> dbSaved(reader.readDatabase(m_tmpFileName, key));
    QVERIFY(dbSaved);
    QVERIFY(!reader.hasError());
    QCOMPARE(dbSaved->metadata()->name(), m_db->metadata()->name());
}

void TestGui::triggerAction(const QString& name)
{
    QAction* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
}

void TestGui::dragAndDropGroup(const QModelIndex& sourceIndex, const QModelIndex& targetIndex, int row,
                               bool expectedResult, const QString& expectedParentName, int expectedPos)
{
    QVERIFY(sourceIndex.isValid());
    QVERIFY(targetIndex.isValid());

    GroupModel* groupModel = qobject_cast<GroupModel*>(m_dbWidget->findChild<GroupView*>("groupView")->model());

    QMimeData mimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    Group* group = groupModel->groupFromIndex(sourceIndex);
    stream << group->database()->uuid() << group->uuid();
    mimeData.setData("application/x-keepassx-group", encoded);

    QCOMPARE(groupModel->dropMimeData(&mimeData, Qt::MoveAction, row, 0, targetIndex), expectedResult);
    QCOMPARE(group->parentGroup()->name(), expectedParentName);
    QCOMPARE(group->parentGroup()->children().indexOf(group), expectedPos);
}

void TestGui::clickIndex(const QModelIndex& index, QAbstractItemView* view, Qt::MouseButton button,
                         Qt::KeyboardModifiers stateKey)
{
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}

QTEST_MAIN(TestGui)
