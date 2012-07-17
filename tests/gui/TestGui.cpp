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

#include <QtTest/QTest>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "keys/PasswordKey.h"

void TestGui::initTestCase()
{
    Crypto::init();
    Config::createTempFileInstance();
    m_mainWindow = new MainWindow();
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();
    QTest::qWaitForWindowShown(m_mainWindow);
}

void TestGui::testOpenDatabase()
{
    QAction* actionDatabaseOpen = m_mainWindow->findChild<QAction*>("actionDatabaseOpen");
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    actionDatabaseOpen->trigger();
    QTest::qWait(20);

    QWidget* databaseOpenWidget = m_mainWindow->findChild<QWidget*>("databaseOpenWidget");
    QLineEdit* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QTest::qWait(20);
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
    QTest::qWait(20);

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QTest::qWait(20);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QVERIFY(m_dbWidget->currentWidget() == editEntryWidget);
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(200); // wait for modified timer
    // make sure the database isn't marked as modified
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("NewDatabase.kdbx"));
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
    QTest::qWait(20);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    QTest::qWait(20);

    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->historyItems().size(), 0);
    QTest::qWait(200); // wait for modified timer
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("NewDatabase.kdbx*"));

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);
    QTest::keyClicks(titleEdit, "something");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(entry->title(), QString("testsomething"));
    QCOMPARE(entry->historyItems().size(), 1);
}

void TestGui::testSearch()
{
    QAction* searchAction = m_mainWindow->findChild<QAction*>("actionSearch");
    QVERIFY(searchAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* searchActionWidget = toolBar->widgetForAction(searchAction);
    QVERIFY(searchActionWidget->isVisible());
    QVERIFY(searchActionWidget->isEnabled());
    QTest::mouseClick(searchActionWidget, Qt::LeftButton);
    QTest::qWait(20);

    EntryView* entryView = m_dbWidget->findChild<EntryView*>("entryView");
    QLineEdit* searchEdit = m_dbWidget->findChild<QLineEdit*>("searchEdit");
    QToolButton* clearSearch = m_dbWidget->findChild<QToolButton*>("clearButton");

    QTest::keyClicks(searchEdit, "ZZZ");
    QTest::qWait(200);

    QCOMPARE(entryView->model()->rowCount(), 0);

    QTest::mouseClick(clearSearch, Qt::LeftButton);
    QTest::keyClicks(searchEdit, "some");
    QTest::qWait(200);

    QCOMPARE(entryView->model()->rowCount(), 2);

    QModelIndex item = entryView->model()->index(0, 1);
    QRect itemRect = entryView->visualRect(item);
    QTest::mouseClick(entryView->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::ViewMode);

    QModelIndex item2 = entryView->model()->index(1, 0);
    QRect itemRect2 = entryView->visualRect(item2);
    QTest::mouseClick(entryView->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect2.center());
    QAction* entryDeleteAction = m_mainWindow->findChild<QAction*>("actionEntryDelete");

    QWidget* entryDeleteWidget = toolBar->widgetForAction(entryDeleteAction);
    QVERIFY(entryDeleteWidget->isVisible());
    QVERIFY(entryDeleteWidget->isEnabled());

    QTest::mouseClick(entryDeleteWidget, Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(entryView->model()->rowCount(), 1);
}

void TestGui::testSaveAs()
{
    QFileInfo fileInfo(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QDateTime lastModified = fileInfo.lastModified();

    m_db->metadata()->setName("SaveAs");

    // open temporary file so it creates a filename
    QVERIFY(m_tmpFile.open());
    m_tmpFile.close();
    fileDialog()->setNextFileName(m_tmpFile.fileName());

    QAction* actionDatabaseSaveAs = m_mainWindow->findChild<QAction*>("actionDatabaseSaveAs");
    actionDatabaseSaveAs->trigger();

    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("SaveAs"));

    QVERIFY(checkDatabase());

    fileInfo.refresh();
    QCOMPARE(fileInfo.lastModified(), lastModified);
}

void TestGui::testSave()
{
    m_db->metadata()->setName("Save");
    QTest::qWait(200); // wait for modified timer
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));

    QAction* actionDatabaseSave = m_mainWindow->findChild<QAction*>("actionDatabaseSave");
    actionDatabaseSave->trigger();
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    QVERIFY(checkDatabase());
}

void TestGui::testDatabaseSettings()
{
    QAction* actionChangeDatabaseSettings = m_mainWindow->findChild<QAction*>("actionChangeDatabaseSettings");
    actionChangeDatabaseSettings->trigger();
    QWidget* dbSettingsWidget = m_dbWidget->findChild<QWidget*>("databaseSettingsWidget");
    QSpinBox* transformRoundsSpinBox = dbSettingsWidget->findChild<QSpinBox*>("transformRoundsSpinBox");
    transformRoundsSpinBox->setValue(100);
    QTest::keyClick(transformRoundsSpinBox, Qt::Key_Enter);
    QTest::qWait(200); // wait for modified timer
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save*"));
    QCOMPARE(m_db->transformRounds(), Q_UINT64_C(100));

    QAction* actionDatabaseSave = m_mainWindow->findChild<QAction*>("actionDatabaseSave");
    actionDatabaseSave->trigger();
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("Save"));

    QVERIFY(checkDatabase());
}

void TestGui::testKeePass1Import()
{
    QAction* actionImportKeePass1 = m_mainWindow->findChild<QAction*>("actionImportKeePass1");
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb"));
    actionImportKeePass1->trigger();
    QTest::qWait(20);

    QWidget* keepass1OpenWidget = m_mainWindow->findChild<QWidget*>("keepass1OpenWidget");
    QLineEdit* editPassword = keepass1OpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);

    QTest::keyClicks(editPassword, "masterpw");
    QTest::keyClick(editPassword, Qt::Key_Enter);
    QTest::qWait(20);

    QCOMPARE(m_tabWidget->count(), 2);
    QCOMPARE(m_tabWidget->tabText(m_tabWidget->currentIndex()), QString("basic [New database]*"));
}

void TestGui::cleanupTestCase()
{
    delete m_mainWindow;
}

bool TestGui::checkDatabase()
{
    CompositeKey key;
    key.addKey(PasswordKey("a"));
    KeePass2Reader reader;
    QScopedPointer<Database> dbSaved(reader.readDatabase(m_tmpFile.fileName(), key));
    if (!dbSaved || reader.hasError()) {
        return false;
    }
    if (dbSaved->metadata()->name() != m_db->metadata()->name()) {
        return false;
    }
    return true;
}

KEEPASSX_QTEST_GUI_MAIN(TestGui)
