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

#include <QClipboard>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QToolBar>

#include "gui/PasswordWidget.h"
#include "gui/TotpDialog.h"
#include "gui/TotpSetupDialog.h"
#include "gui/entry/EditEntryWidget.h"

#define VERIFY_SQUARE(rect, message) QVERIFY2(rect.width() == rect.height(), message);

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

void TestGuiTotp::testTotpSetup()
{
    selectEntryViewRow(0);

    triggerAction("actionEntrySetupTotp");

    auto* setupTotpDialog = m_dbWidget->findChild<TotpSetupDialog*>("TotpSetupDialog");

    QApplication::processEvents();

    QString exampleSeed = "gezd gnbvgY 3tqojqGEZdgnb vgy3tqoJq===";
    QString expectedFinalSeed = exampleSeed.toUpper().remove(" ").remove("=");
    auto* seedEdit = setupTotpDialog->findChild<QLineEdit*>("seedEdit");
    seedEdit->setText("");
    QTest::keyClicks(seedEdit, exampleSeed);

    clickDialogButton(setupTotpDialog, QDialogButtonBox::Ok);
    QTRY_VERIFY(!setupTotpDialog->isVisible());

    // Make sure the entryView is selected and active
    auto* entryView = findEntryView();
    entryView->activateWindow();
    QApplication::processEvents();
    QTRY_VERIFY(entryView->hasFocus());

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
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

    clickDialogButton(editEntryWidget, QDialogButtonBox::Ok);
}

void TestGuiTotp::testTotpValue()
{
    auto* entry = selectEntryViewRow(0);
    triggerAction("actionEntryTotp");

    auto* totpDialog = m_dbWidget->findChild<TotpDialog*>("TotpDialog");
    auto* totpLabel = totpDialog->findChild<QLabel*>("totpLabel");

    QCOMPARE(totpLabel->text().replace(" ", ""), entry->totp());

    // Test the clipboard copy
    clickDialogButton(totpDialog, QDialogButtonBox::Ok);
    QCOMPARE(QApplication::clipboard()->text(), entry->totp());
    QVERIFY2(totpDialog->isVisible(), "OK button should not close the TOTP dialog.");

    QTest::keyClick(totpDialog, Qt::Key_Escape);
}

void TestGuiTotp::testQrCode()
{
    selectEntryViewRow(0);

    // Given an open QR code dialog.
    triggerAction("actionEntryTotpQRCode");
    auto* qrCodeDialog = m_mainWindow->findChild<QDialog*>("entryQrCodeWidget");
    QVERIFY(qrCodeDialog);
    QVERIFY(qrCodeDialog->isVisible());
    auto* qrCodeWidget = qrCodeDialog->findChild<QWidget*>("squareSvgWidget");

    // Test the default QR code widget shape.
    VERIFY_SQUARE(qrCodeWidget->geometry(), "Initial QR code is not square")

    // Test the resized QR code widget, make the dialog bigger.
    qrCodeDialog->setFixedSize(800, 600);
    VERIFY_SQUARE(qrCodeWidget->geometry(), "Resized QR code is not square")

    // Test the clipboard copy
    clickDialogButton(qrCodeDialog, QDialogButtonBox::Ok);
    QCOMPARE(
        QApplication::clipboard()->text(),
        "otpauth://totp/"
        "Sample%20Entry:User%20Name?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&period=30&digits=6&issuer=Sample%20Entry");
    QVERIFY2(qrCodeDialog->isVisible(), "OK button should not close the TOTP dialog.");

    // Cleanup, close the QR code dialog.
    QTest::keyClick(qrCodeDialog, Qt::Key_Escape);
}
