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

#include "config-keepassx-tests.h"
#include "tests.h"
#include "crypto/Crypto.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"

void TestGui::initTestCase()
{
    Crypto::init();
    m_mainWindow = new MainWindow();
}

void TestGui::testOpenDatabase()
{
    m_mainWindow->show();
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
}

void TestGui::testTabs()
{
    QTabWidget* tabWidget = m_mainWindow->findChild<QTabWidget*>("tabWidget");
    QCOMPARE(tabWidget->count(), 1);
    QCOMPARE(tabWidget->tabText(tabWidget->currentIndex()), QString("NewDatabase.kdbx"));
}

void TestGui::cleanupTestCase()
{
   delete m_mainWindow;
}

KEEPASSX_QTEST_GUI_MAIN(TestGui)
