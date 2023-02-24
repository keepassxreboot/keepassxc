/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "TestGuiTotp.h"
#include "gui/Application.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QToolBar>

#include "config-keepassx-tests.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
#include "gui/TotpDialog.h"
#include "gui/TotpSetupDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setApplicationVersion(KEEPASSXC_VERSION);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    app.applyTheme();
    QTEST_DISABLE_KEYPAD_NAVIGATION
    TestGuiTotp tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

void TestGuiTotp::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    Application::bootstrap();

    m_mainWindow.reset(new MainWindow());
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_statusBarLabel = m_mainWindow->findChild<QLabel*>("statusBarLabel");
    m_mainWindow->show();
    m_mainWindow->resize(1024, 768);
}

// Every test starts with resetting config settings and opening the temp database
void TestGuiTotp::init()
{
    // Reset config to defaults
    config()->resetToDefaults();
    // Disable autosave so we can test the modified file indicator
    config()->set(Config::AutoSaveAfterEveryChange, false);
    config()->set(Config::AutoSaveOnExit, false);
    // Enable the tray icon so we can test hiding/restoring the windowQByteArray
    config()->set(Config::GUI_ShowTrayIcon, true);
    // Disable the update check first time alert
    config()->set(Config::UpdateCheckMessageShown, true);
    // Disable quick unlock
    config()->set(Config::Security_QuickUnlock, false);
    // Disable atomic saves to prevent transient errors on some platforms
    config()->set(Config::UseAtomicSaves, false);
    // Disable showing expired entries on unlock
    config()->set(Config::GUI_ShowExpiredEntriesOnDatabaseUnlock, false);

    // Copy the test database file to the temporary file
    auto origFilePath = QDir(KEEPASSX_TEST_DATA_DIR).absoluteFilePath("NewDatabase.kdbx");
    QVERIFY(m_dbFile.copyFromFile(origFilePath));

    m_dbFileName = QFileInfo(m_dbFile.fileName()).fileName();
    m_dbFilePath = m_dbFile.fileName();

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    QApplication::processEvents();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");

    QApplication::processEvents();

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    auto* databaseOpenWidget = m_tabWidget->currentDatabaseWidget()->findChild<QWidget*>("databaseOpenWidget");
    QVERIFY(databaseOpenWidget);
    // editPassword is not QLineEdit anymore but PasswordWidget
    auto* editPassword =
        databaseOpenWidget->findChild<PasswordWidget*>("editPassword")->findChild<QLineEdit*>("passwordEdit");
    QVERIFY(editPassword);
    editPassword->setFocus();

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    QTRY_VERIFY(!m_dbWidget->isLocked());
    m_db = m_dbWidget->database();

    QApplication::processEvents();
}

// Every test ends with closing the temp database without saving
void TestGuiTotp::cleanup()
{
    // DO NOT save the database
    m_db->markAsClean();
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");
    QApplication::processEvents();
    MessageBox::setNextAnswer(MessageBox::NoButton);

    delete m_dbWidget;
}

void TestGuiTotp::cleanupTestCase()
{
    m_dbFile.remove();
}

void TestGuiTotp::testTotp()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    QCOMPARE(entryView->model()->rowCount(), 1);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::ViewMode);
    QModelIndex item = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(item);
    clickIndex(item, entryView, Qt::LeftButton);

    triggerAction("actionEntrySetupTotp");

    auto* setupTotpDialog = m_dbWidget->findChild<TotpSetupDialog*>("TotpSetupDialog");

    QApplication::processEvents();

    QString exampleSeed = "gezd gnbvgY 3tqojqGEZdgnb vgy3tqoJq===";
    QString expectedFinalSeed = exampleSeed.toUpper().remove(" ").remove("=");
    auto* seedEdit = setupTotpDialog->findChild<QLineEdit*>("seedEdit");
    seedEdit->setText("");
    QTest::keyClicks(seedEdit, exampleSeed);

    auto* setupTotpButtonBox = setupTotpDialog->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(setupTotpButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTRY_VERIFY(!setupTotpDialog->isVisible());

    // Make sure the entryView is selected and active
    entryView->activateWindow();
    QApplication::processEvents();
    QTRY_VERIFY(entryView->hasFocus());

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QVERIFY(entryEditWidget->isVisible());
    QVERIFY(entryEditWidget->isEnabled());
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);

    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
    editEntryWidget->setCurrentPage(1);
    auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
    QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("revealAttributeButton"), Qt::LeftButton);
    QCOMPARE(attrTextEdit->toPlainText(), expectedFinalSeed);

    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QTest::mouseClick(editEntryWidgetButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

    // Test the TOTP value
    triggerAction("actionEntryTotp");

    auto* totpDialog = m_dbWidget->findChild<TotpDialog*>("TotpDialog");
    auto* totpLabel = totpDialog->findChild<QLabel*>("totpLabel");

    QCOMPARE(totpLabel->text().replace(" ", ""), entry->totp());
    QTest::keyClick(totpDialog, Qt::Key_Escape);

    // Test the QR code
    triggerAction("actionEntryTotpQRCode");
    auto* qrCodeDialog = m_mainWindow->findChild<QDialog*>("entryQrCodeWidget");
    QVERIFY(qrCodeDialog);
    QVERIFY(qrCodeDialog->isVisible());
    auto* qrCodeWidget = qrCodeDialog->findChild<QWidget*>("squareSvgWidget");
    QVERIFY2(qrCodeWidget->geometry().width() == qrCodeWidget->geometry().height(), "Initial QR code is not square");

    // Test the QR code window resizing, make the dialog bigger.
    qrCodeDialog->setFixedSize(800, 600);
    QVERIFY2(qrCodeWidget->geometry().width() == qrCodeWidget->geometry().height(), "Resized QR code is not square");
    QTest::keyClick(qrCodeDialog, Qt::Key_Escape);
}

void TestGuiTotp::triggerAction(const QString& name)
{
    auto* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QApplication::processEvents();
}

void TestGuiTotp::clickIndex(const QModelIndex& index,
                             QAbstractItemView* view,
                             Qt::MouseButton button,
                             Qt::KeyboardModifiers stateKey)
{
    view->scrollTo(index);
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}
