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
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QSignalSpy>

#include "config-keepassx-tests.h"
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
    m_mainWindow->activateWindow();
    Tools::wait(50);

    // Load the NewDatabase.kdbx file into temporary storage
    QByteArray tmpData;
    QFile sourceDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, tmpData));
    sourceDbFile.close();

    // Write the temp storage to a temp database file for use in our tests
    QVERIFY(m_dbFile.open());
    QCOMPARE(m_dbFile.write(tmpData), static_cast<qint64>((tmpData.size())));
    m_dbFile.close();

    m_dbFileName = QFileInfo(m_dbFile).fileName();
}

// Every test starts with opening the temp database
void TestGui::init()
{
    fileDialog()->setNextFileName(m_dbFile.fileName());
    triggerAction("actionDatabaseOpen");

    QWidget* databaseOpenWidget = m_mainWindow->findChild<QWidget*>("databaseOpenWidget");
    QLineEdit* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);
    Tools::wait(100);

    QVERIFY(m_tabWidget->currentDatabaseWidget());

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

// Every test ends with closing the temp database without saving
void TestGui::cleanup()
{
    // DO NOT save the database
    MessageBox::setNextAnswer(QMessageBox::No);
    triggerAction("actionDatabaseClose");
    Tools::wait(100);

    m_db = nullptr;
    m_dbWidget = nullptr;
}

void TestGui::testMergeDatabase()
{
    // this triggers a warning. Perhaps similar to https://bugreports.qt.io/browse/QTBUG-49623 ?
    QSignalSpy dbMergeSpy(m_tabWidget->currentWidget(), SIGNAL(databaseMerged(Database*)));

    // set file to merge from
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx"));
    triggerAction("actionDatabaseMerge");

    QWidget* databaseOpenMergeWidget = m_mainWindow->findChild<QWidget*>("databaseOpenMergeWidget");
    QLineEdit* editPasswordMerge = databaseOpenMergeWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPasswordMerge->isVisible());

    m_tabWidget->currentDatabaseWidget()->setCurrentWidget(databaseOpenMergeWidget);

    QTest::keyClicks(editPasswordMerge, "a");
    QTest::keyClick(editPasswordMerge, Qt::Key_Enter);

    QTRY_COMPARE(dbMergeSpy.count(), 1);
    QTRY_VERIFY(m_tabWidget->tabText(m_tabWidget->currentIndex()).contains("*"));

    m_db = m_tabWidget->currentDatabaseWidget()->database();

    // there are seven child groups of the root group
    QCOMPARE(m_db->rootGroup()->children().size(), 7);
    // the merged group should contain an entry
    QCOMPARE(m_db->rootGroup()->children().at(6)->entries().size(), 1);
    // the General group contains one entry merged from the other db
    QCOMPARE(m_db->rootGroup()->findChildByName("General")->entries().size(), 1);
}

void TestGui::testTabs()
{
    QCOMPARE(m_tabWidget->count(), 1);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), m_dbFileName);
}

void TestGui::testEditEntry()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Select the first entry in the database
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QModelIndex entryItem = entryView->model()->index(0, 1);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    // Confirm the edit action button is enabled
    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());

    // Edit the first entry ("Sample Entry")
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "_test");

    // Save the edit
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm edit was made
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    Entry* entry = entryView->entryFromIndex(entryItem);
    QCOMPARE(entry->title(), QString("Sample Entry_test"));
    QCOMPARE(entry->historyItems().size(), 1);

    // Confirm modified indicator is showing
    QTRY_COMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("%1*").arg(m_dbFileName));
}

void TestGui::testAddEntry()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
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

    // Add entry "something 2"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 2");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Add entry "something 3"
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::keyClicks(titleEdit, "something 3");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm that 4 entries now exist
    QTRY_COMPARE(entryView->model()->rowCount(), 4);
}

void TestGui::testEntryEntropy()
{
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    // Find the new entry action
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());

    // Find the button associated with the new entry action
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    // Click the new entry button and check that we enter edit mode
    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Add entry "test" and confirm added
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");

    // Open the password generator
    QToolButton* generatorButton = editEntryWidget->findChild<QToolButton*>("tooglePasswordGeneratorButton");
    QTest::mouseClick(generatorButton, Qt::LeftButton);

    // Type in some password
    QLineEdit* editNewPassword = editEntryWidget->findChild<QLineEdit*>("editNewPassword");
    QLabel* entropyLabel = editEntryWidget->findChild<QLabel*>("entropyLabel");

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "hello");
    QCOMPARE(entropyLabel->text(), QString("6.38 bits"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "helloworld");
    QCOMPARE(entropyLabel->text(), QString("13.10 bits"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "password1");
    QCOMPARE(entropyLabel->text(), QString("4.00 bits"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "D0g..................");
    QCOMPARE(entropyLabel->text(), QString("19.02 bits"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "Tr0ub4dour&3");
    QCOMPARE(entropyLabel->text(), QString("30.87 bits"));

    editNewPassword->setText("");
    QTest::keyClicks(editNewPassword, "correcthorsebatterystaple");
    QCOMPARE(entropyLabel->text(),  QString("47.98 bits"));
    // We are done
}

void TestGui::testSearch()
{
    // Add canned entries for consistent testing
    testAddEntry();

    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");

    QWidget* searchActionWidget = toolBar->findChild<QWidget*>("SearchWidget");
    QVERIFY(searchActionWidget->isEnabled());
    QLineEdit* searchEdit = searchActionWidget->findChild<QLineEdit*>("searchEdit");

    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QVERIFY(entryView->isVisible());

    // Enter search
    QTest::mouseClick(searchEdit, Qt::LeftButton);
    QTRY_VERIFY(searchEdit->hasFocus());
    // Search for "ZZZ"
    QTest::keyClicks(searchEdit, "ZZZ");
    QTRY_COMPARE(searchEdit->text(), QString("ZZZ"));
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QTRY_COMPARE(entryView->model()->rowCount(), 0);
    // Escape clears searchedit and retains focus
    QTest::keyClick(searchEdit, Qt::Key_Escape);
    QTRY_VERIFY(searchEdit->text().isEmpty());
    QTRY_VERIFY(searchEdit->hasFocus());
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    // Search for "some"
    QTest::keyClicks(searchEdit, "some");
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QTRY_COMPARE(entryView->model()->rowCount(), 3);
    // Press Down to focus on the entry view
    QTest::keyClicks(searchEdit, "thing");
    QTRY_COMPARE(entryView->model()->rowCount(), 2);
    //QVERIFY(!entryView->hasFocus());
    //QTest::keyClick(searchEdit, Qt::Key_Down);
    //QVERIFY(entryView->hasFocus());

    // Try to edit the first entry from the search view
    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);
    QVERIFY(m_dbWidget->isInSearchMode());
    clickIndex(item, entryView, Qt::LeftButton);
    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    // Perform the edit and save it
    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QString origTitle = titleEdit->text();
    QTest::keyClicks(titleEdit, "_edited");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Confirm the edit was made and we are back in view mode
    QTRY_VERIFY(m_dbWidget->isInSearchMode());
    QCOMPARE(entry->title(), origTitle.append("_edited"));

    // Cancel search, should return to normal view
    QTest::mouseClick(searchEdit, Qt::LeftButton);
    QTest::keyClick(searchEdit, Qt::Key_Escape);
    QTRY_COMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    //QCOMPARE(entryView->model()->rowCount(), 4);

    // TODO: add tests to confirm case sensitive and group search
}

void TestGui::testDeleteEntry()
{
    // Add canned entries for consistent testing
    testAddEntry();

    GroupView* groupView = m_dbWidget->findChild<GroupView*>("groupView");
    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QAction* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");
    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    clickIndex(entryView->model()->index(1, 0), entryView, Qt::LeftButton);
    QVERIFY(entryDeleteWidget->isVisible());
    QVERIFY(entryDeleteWidget->isEnabled());
    QVERIFY(!m_db->metadata()->recycleBin());

    MessageBox::setNextAnswer(QMessageBox::Yes);
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
                     -1, true, "NewDatabase", 4);
}

void TestGui::testSaveAs()
{
    QFileInfo fileInfo(m_dbFile.fileName());
    QDateTime lastModified = fileInfo.lastModified();

    m_db->metadata()->setName("SaveAs");

    // open temporary file so it creates a filename
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QString tmpFileName = tmpFile.fileName();
    tmpFile.remove();

    fileDialog()->setNextFileName(tmpFileName);

    triggerAction("actionDatabaseSaveAs");

    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("SaveAs"));

    checkDatabase(tmpFileName);

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

    // Close the KeePass1 Database
    MessageBox::setNextAnswer(QMessageBox::No);
    triggerAction("actionDatabaseClose");
    Tools::wait(100);

}

void TestGui::testDatabaseLocking()
{
    QString origDbName = m_tabWidget->tabText(0);

    MessageBox::setNextAnswer(QMessageBox::Cancel);
    triggerAction("actionLockDatabases");

    QCOMPARE(m_tabWidget->tabText(0).remove('&'), origDbName + " [locked]");

    QWidget* dbWidget = m_tabWidget->currentDatabaseWidget();
    QWidget* unlockDatabaseWidget = dbWidget->findChild<QWidget*>("unlockDatabaseWidget");
    QWidget* editPassword = unlockDatabaseWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QCOMPARE(m_tabWidget->tabText(0).remove('&'), origDbName);
}

void TestGui::cleanupTestCase()
{
    delete m_mainWindow;
}

void TestGui::checkDatabase(QString dbFileName)
{
    if (dbFileName.isEmpty())
        dbFileName = m_dbFile.fileName();

    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    QScopedPointer<Database> dbSaved(reader.readDatabase(dbFileName, key));
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
