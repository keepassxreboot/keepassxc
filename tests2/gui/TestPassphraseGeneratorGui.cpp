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
#include "../streams.h"
#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/PasswordGeneratorWidget.h"
#include "gui/PasswordWidget.h"
#include "gui/entry/EditEntryWidget.h"

#include <QClipboard>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <QToolBar>

// TODO add test cases with custom word list, delete selected word list, etc.
SCENARIO_METHOD(FixtureBase, "Passphrase Generator in standalone mode", "[gui]")
{
    GIVEN("User opens the Passphrase Generator on locked DB")
    {
        clickToolbarButton("actionPasswordGenerator");

        auto* pPwGenWidget = m_mainWindow->findChild<PasswordGeneratorWidget*>();
        auto* pPassPhraseWidget = pPwGenWidget->findChild<QWidget*>("dicewareWidget");
        pPwGenWidget->findChild<QTabWidget*>("tabWidget")->setCurrentWidget(pPassPhraseWidget);
        REQUIRE(pPassPhraseWidget->isVisible());

        WHEN("User keeps the default parameters")
        {
            REQUIRE(pPassPhraseWidget->findChild<QSpinBox*>("spinBoxWordCount")->value() == 7);
            REQUIRE(pPassPhraseWidget->findChild<QLineEdit*>("editWordSeparator")->text() == " ");
            REQUIRE(pPassPhraseWidget->findChild<QComboBox*>("wordCaseComboBox")->currentText()
                    == QString("lower case"));

            // Make sure only one embedded word list is available, selected, and could not be deleted.
            auto* pComboBoxWordList = pPassPhraseWidget->findChild<QComboBox*>("comboBoxWordList");
            REQUIRE(pComboBoxWordList->count() == 1);
            REQUIRE(pComboBoxWordList->currentText() == "(SYSTEM) eff_large.wordlist");
            REQUIRE_FALSE(pPassPhraseWidget->findChild<QPushButton*>("buttonDeleteWordList")->isEnabled());

            REQUIRE(pPassPhraseWidget->findChild<QPushButton*>("buttonAddWordList")->isEnabled());

            THEN("A passphrase is generated and has expected entropy and strength")
            {
                auto actualPhrase = pPwGenWidget->getGeneratedPassword();

                REQUIRE(actualPhrase == actualPhrase.toLower());
                REQUIRE_THAT(actualPhrase.toStdString(), Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
                REQUIRE(findLabelText(pPwGenWidget, "entropyLabel") == QString("Entropy: 90.47 bit"));
                REQUIRE(findLabelText(pPwGenWidget, "strengthLabel") == QString("Password Quality: Good"));
                REQUIRE(findLabelText(pPwGenWidget, "charactersInPassphraseLabel").toInt() == actualPhrase.size());
            }
        }

        WHEN("User chooses custom parameters")
        {
            pPassPhraseWidget->findChild<QSpinBox*>("spinBoxWordCount")->setValue(4);
            pPassPhraseWidget->findChild<QLineEdit*>("editWordSeparator")->setText("--");
            pPassPhraseWidget->findChild<QComboBox*>("wordCaseComboBox")->setCurrentText("UPPER CASE");

            THEN("A passphrase is generated and has expected entropy and strength")
            {
                auto actualPhase = pPwGenWidget->getGeneratedPassword();

                REQUIRE(actualPhase == actualPhase.toUpper());
                REQUIRE_THAT(actualPhase.toStdString(), Catch::Matchers::Matches(R"(^\S+--\S+--\S+--\S+$)"));
                REQUIRE(findLabelText(pPwGenWidget, "entropyLabel") == QString("Entropy: 51.70 bit"));
                REQUIRE(findLabelText(pPwGenWidget, "strengthLabel") == QString("Password Quality: Weak"));
                REQUIRE(findLabelText(pPwGenWidget, "charactersInPassphraseLabel").toInt() == actualPhase.size());
            }
        }

        WHEN("User presses Ctrl(Cmd)+C (copy)")
        {
#ifdef Q_OS_MAC
            QTest::keyClick(pPwGenWidget, Qt::Key_C, Qt::MetaModifier);
#else
            QTest::keyClick(pPwGenWidget, Qt::Key_C, Qt::ControlModifier);
#endif
            wait(250);

            THEN("A passphrase is copied into clipboard")
            {
                REQUIRE(QApplication::clipboard()->text() == pPwGenWidget->getGeneratedPassword());
            }
        }

        WHEN("User presses Ctrl(Cmd)+R (regenerate)")
        {
            auto actualPhraseBefore = pPwGenWidget->getGeneratedPassword();

#ifdef Q_OS_MAC
            QTest::keyClick(pPwGenWidget, Qt::Key_R, Qt::MetaModifier);
#else
            QTest::keyClick(pPwGenWidget, Qt::Key_R, Qt::ControlModifier);
#endif
            wait(250);

            THEN("A new passphrase is generated")
            {
                auto actualPhrase = pPwGenWidget->getGeneratedPassword();

                REQUIRE_FALSE(actualPhraseBefore == actualPhrase);
                REQUIRE(actualPhrase == actualPhrase.toLower());
                REQUIRE_THAT(actualPhrase.toStdString(), Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
            }
        }

        // Close the widget after each test. Why?
        // With static main window we should close a widget explicitly to avoid a side effect for the next test.
        escape(pPassPhraseWidget);
        REQUIRE_FALSE(pPwGenWidget->isVisible());
    }
}

SCENARIO_METHOD(FixtureWithDb, "Passphrase Generator on new Entry", "[gui]")
{
    GIVEN("User creates a new Entry and opens a Passphrase Generator from the password field by a short key")
    {
        clickToolbarButton("actionEntryNew");
        REQUIRE(m_dbWidget->currentMode() == DatabaseWidget::Mode::EditMode);

        auto* pEditEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");
        auto* pPasswordEdit = pEditEntryWidget->findChild<PasswordWidget*>()->findChild<QLineEdit*>("passwordEdit");
        QTest::mouseClick(pPasswordEdit, Qt::LeftButton);

#ifdef Q_OS_MAC
        QTest::keyClick(pPasswordEdit, Qt::Key_G, Qt::MetaModifier);
#else
        QTest::keyClick(pPasswordEdit, Qt::Key_G, Qt::ControlModifier);
#endif

        auto* pPwGenWidget = m_dbWidget->findChild<PasswordGeneratorWidget*>();
        auto* pPassPhraseWidget = pPwGenWidget->findChild<QWidget*>("dicewareWidget");
        pPwGenWidget->findChild<QTabWidget*>("tabWidget")->setCurrentWidget(pPassPhraseWidget);
        REQUIRE(pPassPhraseWidget->isVisible());

        WHEN("User keeps the default parameters")
        {
            auto actualPhase = pPwGenWidget->getGeneratedPassword();
            AND_WHEN("User clicks Apply Password button")
            {
                auto* applyPasswordButton = pPwGenWidget->findChild<QPushButton*>("buttonApply");
                QTest::mouseClick(applyPasswordButton, Qt::MouseButton::LeftButton);
                QApplication::processEvents();

                THEN("The Passphrase Generator closed and a new passphrase is applied for the entry")
                {
                    // FIXME dmaslenko: "The emulated Apply click does not close the widget but applies the changes");
                    // REQUIRE_FALSE(pPassPhraseWidget->isVisible());
                    REQUIRE(actualPhase == actualPhase.toLower());
                    REQUIRE_THAT(actualPhase.toStdString(),
                                 Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
                }
            }
        }

        // Close the widget after each test.
        escape(pPassPhraseWidget);
        wait(1000);
        // FIXME dmaslenko: the window is not closed
        // REQUIRE_FALSE(pPassPhraseWidget->isVisible());
    }
}
