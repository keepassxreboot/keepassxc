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
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "crypto/Crypto.h"
#include "core/Entry.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"

void TestGui::initTestCase()
{
    Crypto::init();
    m_mainWindow = new MainWindow();
    m_mainWindow->show();
    QTest::qWaitForWindowShown(m_mainWindow);
}

void TestGui::testOpenDatabase()
{
    QAction* actionDatabaseOpen = m_mainWindow->findChild<QAction*>("actionDatabaseOpen");
    fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    actionDatabaseOpen->trigger();
    QWidget* keyDialog = m_mainWindow->findChild<QWidget*>("DatabaseOpenDialog");
    QVERIFY(keyDialog);
    QTest::qWaitForWindowShown(keyDialog);

    QLineEdit* editPassword = keyDialog->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);
    QTest::keyClicks(editPassword, "a");

    QDialogButtonBox* buttonBox = keyDialog->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(buttonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);
}

void TestGui::testTabs()
{
    QTabWidget* tabWidget = m_mainWindow->findChild<QTabWidget*>("tabWidget");
    QCOMPARE(tabWidget->count(), 1);
    QCOMPARE(tabWidget->tabText(tabWidget->currentIndex()), QString("NewDatabase.kdbx"));
}

void TestGui::testEditEntry()
{
    DatabaseTabWidget* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    DatabaseWidget* dbWidget = tabWidget->currentDatabaseWidget();
    EntryView* entryView = dbWidget->findChild<EntryView*>("entryView");
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

    EditEntryWidget* editEntryWidget = dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QVERIFY(dbWidget->currentWidget() == editEntryWidget);
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);
    // make sure the database isn't marked as modified
    QCOMPARE(tabWidget->tabText(tabWidget->currentIndex()), QString("NewDatabase.kdbx"));
}

void TestGui::testAddEntry()
{
    DatabaseTabWidget* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    DatabaseWidget* dbWidget = tabWidget->currentDatabaseWidget();

    EntryView* entryView = dbWidget->findChild<EntryView*>("entryView");
    QAction* entryNewAction = m_mainWindow->findChild<QAction*>("actionEntryNew");
    QVERIFY(entryNewAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* entryNewWidget = toolBar->widgetForAction(entryNewAction);
    QVERIFY(entryNewWidget->isVisible());
    QVERIFY(entryNewWidget->isEnabled());

    QTest::mouseClick(entryNewWidget, Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QLineEdit* titleEdit = editEntryWidget->findChild<QLineEdit*>("titleEdit");
    QTest::keyClicks(titleEdit, "test");
    QTest::qWait(20);

    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(dbWidget->currentMode(), DatabaseWidget::ViewMode);
    QModelIndex item = entryView->model()->index(1, 1);
    Entry* entry = entryView->entryFromIndex(item);

    QCOMPARE(entry->title(), QString("test"));
    QCOMPARE(entry->historyItems().size(), 0);
    QCOMPARE(tabWidget->tabText(tabWidget->currentIndex()), QString("NewDatabase.kdbx*"));

    QAction* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QVERIFY(entryEditAction->isEnabled());
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(dbWidget->currentMode(), DatabaseWidget::EditMode);
    QTest::keyClicks(titleEdit, "something");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(entry->title(), QString("testsomething"));
    QCOMPARE(entry->historyItems().size(), 1);
}

void TestGui::testSearch()
{
    DatabaseTabWidget* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    DatabaseWidget* dbWidget = tabWidget->currentDatabaseWidget();

    QAction* serachAction = m_mainWindow->findChild<QAction*>("actionSearch");
    QVERIFY(serachAction->isEnabled());
    QToolBar* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    QWidget* serachActionWidget = toolBar->widgetForAction(serachAction);
    QVERIFY(serachActionWidget->isVisible());
    QVERIFY(serachActionWidget->isEnabled());
    QTest::mouseClick(serachActionWidget, Qt::LeftButton);
    QTest::qWait(20);

    EntryView* entryView = dbWidget->findChild<EntryView*>("entryView");
    QLineEdit* searchEdit = dbWidget->findChild<QLineEdit*>("searchEdit");
    QToolButton* clearSearch = dbWidget->findChild<QToolButton*>("clearButton");

    QTest::keyClicks(searchEdit, "ZZZ");
    QTest::qWait(120);

    QCOMPARE(entryView->model()->rowCount(), 0);

    QTest::mouseClick(clearSearch, Qt::LeftButton);
    QTest::keyClicks(searchEdit, "some");
    QTest::qWait(120);

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

    QCOMPARE(dbWidget->currentMode(), DatabaseWidget::EditMode);

    EditEntryWidget* editEntryWidget = dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    QDialogButtonBox* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTest::qWait(20);

    QCOMPARE(dbWidget->currentMode(), DatabaseWidget::ViewMode);

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

void TestGui::cleanupTestCase()
{
    delete m_mainWindow;
}

KEEPASSX_QTEST_GUI_MAIN(TestGui)
