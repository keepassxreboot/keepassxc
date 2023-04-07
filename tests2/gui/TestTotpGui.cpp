/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#include "../streams.h" // for printable logs on QStrings verification

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/TotpDialog.h"
#include "gui/TotpSetupDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"
#include "core/Totp.h"

#include <QClipboard>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTest>
#include <QToolBar>

SCENARIO_METHOD(FixtureWithDb, "Test TOTP as entry attribute", "[gui]")
{
    WHEN("User opens the entry attributes to check TOTP seed")
    {
        triggerAction("actionEntryEdit");

        auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
        REQUIRE(editEntryWidget);
        editEntryWidget->setCurrentPage(1);

        QTest::mouseClick(editEntryWidget->findChild<QAbstractButton*>("revealAttributeButton"), Qt::LeftButton);

        THEN("Revealed TOTP seed attribute has expected value")
        {
            auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");
            REQUIRE(attrTextEdit->toPlainText() == QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test TOTP setup functionality", "[gui]")
{
    WHEN("User opens the TOTP setup dialog")
    {
        triggerAction("actionEntrySetupTotp");

        auto* pTotpSetupDialog = m_dbWidget->findChild<TotpSetupDialog*>();
        REQUIRE(pTotpSetupDialog);

        THEN("The dialog has expected values")
        {
            QString originalSeed = getCurrentEntry()->totpSettings().data()->key;

            REQUIRE(pTotpSetupDialog->isVisible());
            REQUIRE(pTotpSetupDialog->findChild<QLineEdit*>("seedEdit")->text() == originalSeed);
            REQUIRE(pTotpSetupDialog->findChild<QRadioButton*>("radioDefault")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QRadioButton*>("radioSteam")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QRadioButton*>("radioCustom")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QGroupBox*>("customSettingsGroup")->isEnabled());
        }

        WHEN("User updates the seed")
        {
            QString newSeed = "gezd gnbvgY 3tqojqGEZdgnb vgy3tqoJq===";
            auto* pSeedEdit = pTotpSetupDialog->findChild<QLineEdit*>("seedEdit");
            pSeedEdit->setText("");
            QTest::keyClicks(pSeedEdit, newSeed);

            auto* pSetupTotpButtonBox = pTotpSetupDialog->findChild<QDialogButtonBox*>("buttonBox");

            WHEN("User clicks Ok button")
            {
                QTest::mouseClick(pSetupTotpButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

                THEN("The dialog is closed and the current entry got new TOTP")
                {
                    REQUIRE_FALSE(pTotpSetupDialog->isVisible());

                    QString expectedFinalSeed = newSeed.toUpper().remove(" ").remove("=");
                    REQUIRE(getCurrentEntry()->totpSettings().data()->key == expectedFinalSeed);
                }
            }

            WHEN("User clicks Cancel button")
            {
                QTest::mouseClick(pSetupTotpButtonBox->button(QDialogButtonBox::Cancel), Qt::LeftButton);

                THEN("The dialog is closed and the current entry did not get updated TOTP")
                {
                    QString originalSeed = getCurrentEntry()->totpSettings().data()->key;

                    REQUIRE_FALSE(pTotpSetupDialog->isVisible());
                    REQUIRE(getCurrentEntry()->totpSettings().data()->key == originalSeed);
                }
            }
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test TOTP QR code functionality", "[gui]")
{
    WHEN("User opens the QR code")
    {
        triggerAction("actionEntryTotpQRCode");

        auto* qrCodeDialog = m_dbWidget->findChild<QDialog*>("entryQrCodeWidget");
        REQUIRE(qrCodeDialog);

        auto* qrCodeWidget = qrCodeDialog->findChild<QWidget*>("squareSvgWidget");
        REQUIRE(qrCodeWidget);

        auto* pButtonBox = qrCodeDialog->findChild<QDialogButtonBox*>();
        REQUIRE(pButtonBox);

        THEN("The QR code is open and the shape is a square")
        {
            REQUIRE(qrCodeDialog->isVisible());
            REQUIRE(qrCodeWidget->geometry().width() == qrCodeWidget->geometry().height());
        }

        WHEN("User changes the dialog size")
        {
            qrCodeDialog->setFixedSize(800, 600);

            THEN("The QR code shape is a square")
            {
                REQUIRE(qrCodeWidget->geometry().width() == qrCodeWidget->geometry().height());
            }
        }

        WHEN("User clicks the Copy (Ok) button")
        {
            QTest::mouseClick(pButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

            THEN("The dialog is opened and QR code was copied into the clipboard")
            {
                REQUIRE(qrCodeDialog->isVisible());
                REQUIRE(QApplication::clipboard()->text()
                        == QString("otpauth://totp/"
                                   "Sample%20Entry:User%20Name?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&period="
                                   "30&digits=6&issuer=Sample%20Entry"));
            }
        }

        WHEN("User clicks the Close button")
        {
            QTest::mouseClick(pButtonBox->button(QDialogButtonBox::Close), Qt::LeftButton);

            THEN("The dialog is closed and the clipboard is empty")
            {
                REQUIRE_FALSE(qrCodeDialog->isVisible());
                REQUIRE(QApplication::clipboard()->text().isEmpty());
            }
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test TOTP value functionality", "[gui]")
{
    WHEN("User opens the TOTP value")
    {
        triggerAction("actionEntryTotp");

        auto* totpDialog = m_dbWidget->findChild<TotpDialog*>("TotpDialog");
        REQUIRE(totpDialog);
        REQUIRE(totpDialog->isVisible());

        auto* totpLabel = totpDialog->findChild<QLabel*>("totpLabel");
        REQUIRE(totpLabel);
        REQUIRE(totpLabel->text().replace(" ", "") == getCurrentEntry()->totp());

        auto* pButtonBox = totpDialog->findChild<QDialogButtonBox*>();
        REQUIRE(pButtonBox);

        WHEN("User clicks the Copy (Ok) button")
        {
            QTest::mouseClick(pButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

            THEN("The dialog is opened and TOTP value was copied into the clipboard")
            {
                REQUIRE(totpDialog->isVisible());
                REQUIRE(QApplication::clipboard()->text() == getCurrentEntry()->totp());
            }
        }

        WHEN("User clicks the Close button")
        {
            QTest::mouseClick(pButtonBox->button(QDialogButtonBox::Close), Qt::LeftButton);

            THEN("The dialog is closed and the clipboard is empty")
            {
                REQUIRE_FALSE(totpDialog->isVisible());
                REQUIRE(QApplication::clipboard()->text().isEmpty());
            }
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test TOTP clipboard functionality", "[gui]")
{
    WHEN("User copies the TOTP value")
    {
        triggerAction("actionEntryCopyTotp");
        THEN("The TOTP value was copied into the clipboard")
        {
            REQUIRE(QApplication::clipboard()->text() == getCurrentEntry()->totp());
        }
    }

    WHEN("User copies the Password and TOTP values")
    {
        triggerAction("actionEntryCopyPasswordTotp");
        THEN("The Password and TOTP value were copied into the clipboard")
        {
            REQUIRE(QApplication::clipboard()->text() == getCurrentEntry()->password() + getCurrentEntry()->totp());
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test TOTP setup on new Entry", "[gui]")
{
    auto* pCurrentEntry = newEntry("testEntry", "testUser", "testPassword");

    WHEN("User opens the TOTP Setup dialog")
    {
        triggerAction("actionEntrySetupTotp");

        auto* pTotpSetupDialog = m_dbWidget->findChild<TotpSetupDialog*>();
        REQUIRE(pTotpSetupDialog);

        THEN("The dialog has expected default values")
        {
            REQUIRE(pTotpSetupDialog->isVisible());

            REQUIRE(pTotpSetupDialog->findChild<QLineEdit*>("seedEdit")->text().isEmpty());
            REQUIRE(pTotpSetupDialog->findChild<QRadioButton*>("radioDefault")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QRadioButton*>("radioSteam")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QRadioButton*>("radioCustom")->isChecked());
            REQUIRE_FALSE(pTotpSetupDialog->findChild<QGroupBox*>("customSettingsGroup")->isEnabled());
        }

        WHEN("User fills a seed")
        {
            QString exampleSeed = "gezd gnbvgY 3tqojqGEZdgnb vgy3tqoJq===";
            QTest::keyClicks(pTotpSetupDialog->findChild<QLineEdit*>("seedEdit"), exampleSeed);

            auto* pSetupTotpButtonBox = pTotpSetupDialog->findChild<QDialogButtonBox*>("buttonBox");

            WHEN("User clicks Ok button")
            {
                QTest::mouseClick(pSetupTotpButtonBox->button(QDialogButtonBox::Ok), Qt::LeftButton);

                THEN("The dialog is closed and the current entry got new TOTP")
                {
                    REQUIRE_FALSE(pTotpSetupDialog->isVisible());

                    QString expectedSeed = exampleSeed.toUpper().remove(" ").remove("=");
                    REQUIRE(pCurrentEntry->totpSettings().data()->key == expectedSeed);
                    // TODO: check TOTP icon in the entries table
                }
            }
        }
    }
}
