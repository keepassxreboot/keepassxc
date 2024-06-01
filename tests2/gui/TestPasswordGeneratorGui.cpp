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
#include "core/PasswordHealth.h"
#include "gui/PasswordGeneratorWidget.h"
#include "gui/PasswordWidget.h"
#include "gui/entry/EditEntryWidget.h"

#include <QClipboard>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTest>
#include <QToolBar>

SCENARIO_METHOD(FixtureBase, "Password Generator in standalone mode", "[gui]")
{
    GIVEN("User opens the Password Generator on a locked DB")
    {
        clickToolbarButton("actionPasswordGenerator");

        auto* pPwdGenWidget = m_mainWindow->findChild<PasswordGeneratorWidget*>();
        auto* pPwdWidget = pPwdGenWidget->findChild<QWidget*>("passwordWidget");
        pPwdGenWidget->findChild<QTabWidget*>("tabWidget")->setCurrentWidget(pPwdWidget);
        REQUIRE(pPwdWidget->isVisible());

        auto* pPasswordEdit =
            pPwdGenWidget->findChild<PasswordWidget*>("editNewPassword")->findChild<QLineEdit*>("passwordEdit");

        WHEN("User keeps the default parameters to generate a password")
        {
            REQUIRE(pPwdWidget->findChild<QSlider*>("sliderLength")->value() == 20);
            REQUIRE(pPwdWidget->findChild<QSpinBox*>("spinBoxLength")->value() == 20);
            REQUIRE_FALSE(pPwdWidget->findChild<QPushButton*>("buttonAdvancedMode")->isChecked());

            // Check Character types
            auto* checkBoxUpper = pPwdWidget->findChild<QPushButton*>("checkBoxUpper");
            REQUIRE(checkBoxUpper->isVisible());
            REQUIRE(checkBoxUpper->isChecked());

            auto* checkBoxLower = pPwdWidget->findChild<QPushButton*>("checkBoxLower");
            REQUIRE(checkBoxLower->isVisible());
            REQUIRE(checkBoxLower->isChecked());

            auto* checkBoxNumbers = pPwdWidget->findChild<QPushButton*>("checkBoxNumbers");
            REQUIRE(checkBoxNumbers->isVisible());
            REQUIRE(checkBoxNumbers->isChecked());

            auto* checkBoxSpecialChars = pPwdWidget->findChild<QPushButton*>("checkBoxSpecialChars");
            REQUIRE(checkBoxSpecialChars->isVisible());
            REQUIRE(checkBoxSpecialChars->isChecked());
            // The button name when the Advanced is off.
            REQUIRE(checkBoxSpecialChars->text() == QString("/ * + && …"));

            auto* checkBoxExtASCII = pPwdWidget->findChild<QPushButton*>("checkBoxExtASCII");
            REQUIRE(checkBoxExtASCII->isVisible());
            REQUIRE_FALSE(checkBoxExtASCII->isChecked());

            // The advanced options are not visible and have default state.
            auto* checkBoxPunctuation = pPwdWidget->findChild<QPushButton*>("checkBoxPunctuation");
            REQUIRE_FALSE(checkBoxPunctuation->isVisible());
            REQUIRE_FALSE(checkBoxPunctuation->isChecked());

            auto* checkBoxQuotes = pPwdWidget->findChild<QPushButton*>("checkBoxQuotes");
            REQUIRE_FALSE(checkBoxQuotes->isVisible());
            REQUIRE_FALSE(checkBoxQuotes->isChecked());

            auto* checkBoxDashes = pPwdWidget->findChild<QPushButton*>("checkBoxDashes");
            REQUIRE_FALSE(checkBoxDashes->isVisible());
            REQUIRE_FALSE(checkBoxDashes->isChecked());

            auto* checkBoxMath = pPwdWidget->findChild<QPushButton*>("checkBoxMath");
            REQUIRE_FALSE(checkBoxMath->isVisible());
            REQUIRE_FALSE(checkBoxMath->isChecked());

            auto* checkBoxBraces = pPwdWidget->findChild<QPushButton*>("checkBoxBraces");
            REQUIRE_FALSE(checkBoxBraces->isVisible());
            REQUIRE_FALSE(checkBoxBraces->isChecked());

            auto* editAdditionalChars = pPwdWidget->findChild<QLineEdit*>("editAdditionalChars");
            REQUIRE_FALSE(editAdditionalChars->isVisible());
            REQUIRE(editAdditionalChars->text().isEmpty());

            auto* editExcludedChars = pPwdWidget->findChild<QLineEdit*>("editExcludedChars");
            REQUIRE_FALSE(editExcludedChars->isVisible());
            REQUIRE(editExcludedChars->text().isEmpty());

            auto* buttonAddHex = pPwdWidget->findChild<QPushButton*>("buttonAddHex");
            REQUIRE_FALSE(buttonAddHex->isVisible());

            auto* checkBoxExcludeAlike = pPwdWidget->findChild<QCheckBox*>("checkBoxExcludeAlike");
            REQUIRE_FALSE(checkBoxExcludeAlike->isVisible());
            REQUIRE(checkBoxExcludeAlike->isChecked());

            auto* checkBoxEnsureEvery = pPwdWidget->findChild<QCheckBox*>("checkBoxEnsureEvery");
            REQUIRE_FALSE(checkBoxEnsureEvery->isVisible());
            REQUIRE(checkBoxEnsureEvery->isChecked());

            THEN("A password is generated and has expected entropy and strength")
            {
                // Just make sure a password was generated, see the corresponding unit tests for all test cases.
                // May have a lower/upper letter, a number, a special character, and must be 20 length.
                // "May have" because the Advanced is not chosen and  "Pick characters from every group" is not applied.
                auto regex = R"(^(?=.*[A-Za-z]*)(?=.*\d*)(?=.*[\(\)\[\]\{\}.,:;\"\'-\/\\_\\|!*+<=>?#\$%&@\^`~]*).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));

                REQUIRE_THAT(findLabelText(pPwdGenWidget, "entropyLabel").toStdString(),
                             Catch::Matchers::Matches(R"(^Entropy: [0-9]+[.][0-9]{2} bit$)"));
                REQUIRE(findLabelText(pPwdGenWidget, "strengthLabel") == QString("Password Quality: Excellent"));
            }
        }

        WHEN("User presses Ctrl+R to regenerate the password")
        {
            auto actualPhraseBefore = pPwdGenWidget->getGeneratedPassword();

            // FIXME: under MacOS the Qt::MetaModifier does not work
#ifdef Q_OS_MAC
            QTest::keyClick(pPwdGenWidget, Qt::Key_R, Qt::ControlModifier);
#else
            QTest::keyClick(pPwdGenWidget, Qt::Key_R, Qt::ControlModifier);
#endif
            wait(250);

            THEN("A new password is generated")
            {
                auto actualPhrase = pPwdGenWidget->getGeneratedPassword();

                REQUIRE_FALSE(actualPhraseBefore == actualPhrase);
                REQUIRE_THAT(actualPhrase.toStdString(), Catch::Matchers::Matches(R"(^\S+$)"));
            }
        }

        WHEN("User presses Ctrl+C to copy the password into the clipboard")
        {
            QTest::keyClick(pPwdGenWidget, Qt::Key_C, Qt::ControlModifier);
            wait(250);

            THEN("A password is copied into clipboard")
            {
                REQUIRE(QApplication::clipboard()->text() == pPwdGenWidget->getGeneratedPassword());
            }
        }

        WHEN("User presses Ctrl+H to hide the password")
        {
            auto* pPasswordField = pPwdGenWidget->findChild<PasswordWidget*>("editNewPassword");
            pPasswordField->setFocus();
#ifdef Q_OS_MAC
            QTest::keyClick(pPasswordField, Qt::Key_H, Qt::MetaModifier);
#else
            QTest::keyClick(pPasswordField, Qt::Key_H, Qt::ControlModifier);
#endif
            wait(250);

            THEN("A password is hidden and icon was changed")
            {
                REQUIRE_FALSE(pPasswordField->isPasswordVisible());

                auto* toggleVisibleAction = findAction("passwordToggleVisibleAction");
                REQUIRE(toggleVisibleAction->isEnabled());
                // TODO: how to get a unique icon id or name? The icon name is always empty.
                REQUIRE(toggleVisibleAction->icon().name().isEmpty());
            }
        }

        WHEN("User changes the slide to setup the minimum possible length")
        {
            pPwdWidget->findChild<QSlider*>("sliderLength")->setValue(1);

            THEN("A password is generated with new size")
            {
                auto actualPassword = pPwdGenWidget->getGeneratedPassword();

                // Just make sure a password was generated, see the corresponding unit tests for generation logic.
                REQUIRE(actualPassword.size() == 1);
                REQUIRE_THAT(findLabelText(pPwdGenWidget, "entropyLabel").toStdString(),
                             Catch::Matchers::Matches(R"(^Entropy: [0-9].[0-9]{2} bit$)"));
                REQUIRE(findLabelText(pPwdGenWidget, "strengthLabel") == QString("Password Quality: Poor"));
            }
        }

        WHEN("User clicks on Advanced button to show and apply additional options")
        {
            auto* buttonAdvancedMode = pPwdWidget->findChild<QPushButton*>("buttonAdvancedMode");
            QTest::mouseClick(buttonAdvancedMode, Qt::MouseButton::LeftButton);

            // Note: the below tests under the Advanced section should validate at least one char from each group
            // because almost in all tests (except one) the "Pick characters from every group" is selected.

            // Note: the below tests under the Advanced section should validate for excluding of 0, O, 1, l, I symbol
            // because almost in all tests (except one) the "Exclude look-alike characters" is selected.

            THEN("Advanced options are visible and new password was generated according to default options")
            {
                REQUIRE(buttonAdvancedMode->isChecked());

                auto* checkBoxLogograms = pPwdWidget->findChild<QPushButton*>("checkBoxSpecialChars");
                REQUIRE(checkBoxLogograms->isVisible());
                REQUIRE(checkBoxLogograms->isChecked());
                REQUIRE(checkBoxLogograms->text() == QString("# $ % && @ ^ ` ~"));

                auto* checkBoxPunctuation = pPwdWidget->findChild<QPushButton*>("checkBoxPunctuation");
                REQUIRE(checkBoxPunctuation->isVisible());
                REQUIRE_FALSE(checkBoxPunctuation->isChecked());
                REQUIRE(checkBoxPunctuation->text() == QString(". , : ;"));

                auto* checkBoxQuotes = pPwdWidget->findChild<QPushButton*>("checkBoxQuotes");
                REQUIRE(checkBoxQuotes->isVisible());
                REQUIRE_FALSE(checkBoxQuotes->isChecked());
                REQUIRE(checkBoxQuotes->text() == QString("\" '"));

                auto* checkBoxDashes = pPwdWidget->findChild<QPushButton*>("checkBoxDashes");
                REQUIRE(checkBoxDashes->isVisible());
                REQUIRE_FALSE(checkBoxDashes->isChecked());
                REQUIRE(checkBoxDashes->text() == QString("\\ / | _ -"));

                auto* checkBoxMath = pPwdWidget->findChild<QPushButton*>("checkBoxMath");
                REQUIRE(checkBoxMath->isVisible());
                REQUIRE_FALSE(checkBoxMath->isChecked());
                REQUIRE(checkBoxMath->text() == QString("< > * + ! ? ="));

                auto* checkBoxBraces = pPwdWidget->findChild<QPushButton*>("checkBoxBraces");
                REQUIRE(checkBoxBraces->isVisible());
                REQUIRE_FALSE(checkBoxBraces->isChecked());
                REQUIRE(checkBoxBraces->text() == QString("{ [ ( ) ] }"));

                auto* editAdditionalChars = pPwdWidget->findChild<QLineEdit*>("editAdditionalChars");
                REQUIRE(editAdditionalChars->isVisible());
                REQUIRE(editAdditionalChars->isClearButtonEnabled());
                REQUIRE(editAdditionalChars->text().isEmpty());

                auto* editExcludedChars = pPwdWidget->findChild<QLineEdit*>("editExcludedChars");
                REQUIRE(editExcludedChars->isVisible());
                REQUIRE(editExcludedChars->text().isEmpty());

                auto* buttonAddHex = pPwdWidget->findChild<QPushButton*>("buttonAddHex");
                REQUIRE(buttonAddHex->isVisible());

                auto* checkBoxExcludeAlike = pPwdWidget->findChild<QCheckBox*>("checkBoxExcludeAlike");
                REQUIRE(checkBoxExcludeAlike->isVisible());
                REQUIRE(checkBoxExcludeAlike->isChecked());

                auto* checkBoxEnsureEvery = pPwdWidget->findChild<QCheckBox*>("checkBoxEnsureEvery");
                REQUIRE(checkBoxEnsureEvery->isVisible());
                REQUIRE(checkBoxEnsureEvery->isChecked());

                // Should have a letter and number (excluding look-alike), a special symbol, and be 20 length.
                auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~]).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }

            WHEN("User clicks the '. , : ;' button to get a password with punctuation characters as well")
            {
                auto* checkBoxPunctuation = pPwdWidget->findChild<QPushButton*>("checkBoxPunctuation");
                QTest::mouseClick(checkBoxPunctuation, Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    REQUIRE(checkBoxPunctuation->isChecked());

                    // Should have as with default settings plus punctuation character(s), and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[.,:;]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User clicks the '\" \'' button to get a password with quotes as well")
            {
                auto* checkBoxQuotes = pPwdWidget->findChild<QPushButton*>("checkBoxQuotes");
                QTest::mouseClick(checkBoxQuotes, Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    REQUIRE(checkBoxQuotes->isChecked());

                    // Should have as with default settings plus quote character(s), and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[\"\']).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User clicks the '\\ / | _ -' button to get a password with dashes and slashes")
            {
                auto* checkBoxDashes = pPwdWidget->findChild<QPushButton*>("checkBoxDashes");
                QTest::mouseClick(checkBoxDashes, Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    REQUIRE(checkBoxDashes->isChecked());

                    // Should have as with default settings plus dash/slash character(s), and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[-\/\\_\\|]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User clicks the '< > * + ! ? =' button to get a password with math symbols")
            {
                auto* checkBoxMath = pPwdWidget->findChild<QPushButton*>("checkBoxMath");
                QTest::mouseClick(checkBoxMath, Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    REQUIRE(checkBoxMath->isChecked());

                    // Should have a lower letter, a number, a math character, and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[!*+<=>?]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User clicks the '{ [ ( ) ] }' button to get a password with braces")
            {
                auto* checkBoxBraces = pPwdWidget->findChild<QPushButton*>("checkBoxBraces");
                QTest::mouseClick(checkBoxBraces, Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    REQUIRE(checkBoxBraces->isChecked());

                    // Should have a lower letter, a number, a brace character, and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[\(\)\[\]\{\}]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User provides 'Also choose from' to get a password with custom symbols")
            {
                QTest::keyClicks(pPwdWidget->findChild<QLineEdit*>("editAdditionalChars"), "+-");

                THEN("New password is generated and has expected letters")
                {
                    // Should have an upper/lower letter, a number, a custom characters, and must be 20 length.
                    auto regex = R"(^(?=.*[A-HJ-NP-Z])(?=.*[a-km-z])(?=.*[2-9])(?=.*[#\$%&@\^`~])(?=.*[+-]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User provides 'Do not include' to get a password without specified symbols")
            {
                QTest::keyClicks(pPwdWidget->findChild<QLineEdit*>("editExcludedChars"), "ABCDabcd012");

                THEN("New password is generated and has expected letters")
                {
                    // Should have an upper/lower letter from E to Z only, a number from 3 to 9 only and must be 20 length.
                    auto regex = R"(^(?=.*[E-HJ-NP-Z])(?=.*[e-km-z])(?=.*[3-9])(?=.*[#\$%&@\^`~]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User provides 'Do not include' by Hex button to get a password with Hex symbols only")
            {
                QTest::mouseClick(pPwdWidget->findChild<QPushButton*>("buttonAddHex"), Qt::MouseButton::LeftButton);

                THEN("New password is generated and has expected letters")
                {
                    // Make sure the pressing of the Hex button unchecks the Lower characters button.
                    REQUIRE_FALSE(pPwdWidget->findChild<QPushButton*>("checkBoxLower")->isChecked());

                    // Make sure the pressing of the Hex button adds non-hex letters to "do not include" field.
                    auto* editExcludedChars = pPwdWidget->findChild<QLineEdit*>("editExcludedChars");
                    REQUIRE_THAT(editExcludedChars->text().toStdString(), Catch::Matchers::Matches(R"(^[G-Z]+$)"));

                    // Should have an upper letter A-F only, a number (excluding look-alike), and it must be 20 length.
                    auto regex = R"(^(?=.*[A-F])(?=.*[2-9]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User unchecks 'Exclude look-alike characters' to get a password with 0, O, 1, l, I symbols as well")
            {
                auto* checkBoxExcludeAlike = pPwdWidget->findChild<QCheckBox*>("checkBoxExcludeAlike");
                checkBoxExcludeAlike->setChecked(false);

                THEN("New password is generated and has expected letters")
                {
                    // Should have an upper/lower letter from A to Z, any number, and must be 20 length.
                    auto regex = R"(^(?=.*[A-Z])(?=.*[a-z])(?=.*[0-9])(?=.*[#\$%&@\^`~]).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User unchecks 'Pick characters from every group' to get a password with symbols not from all groups")
            {
                auto* checkBoxEnsureEvery = pPwdWidget->findChild<QCheckBox*>("checkBoxEnsureEvery");
                checkBoxEnsureEvery->setChecked(false);

                THEN("New password is generated and has expected letters")
                {
                    // MAY have an upper/lower letter from A to Z, or number(s), or special symbols and must be 20 length.
                    auto regex = R"(^(?=.*[A-Z]*)(?=.*[a-z]*)(?=.*[0-9]*)(?=.*[#\$%&@\^`~]*).{20}$)";
                    REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
                }
            }

            WHEN("User closes the widget and opens again to get the same selected options")
            {
                escape(pPwdWidget);
                REQUIRE_FALSE(pPwdGenWidget->isVisible());

                clickToolbarButton("actionPasswordGenerator");

                THEN("The visibility of advanced options is saved since last session")
                {
                    REQUIRE(buttonAdvancedMode->isChecked());
                    // Check only couple elements, it is enough.
                    REQUIRE(pPwdWidget->findChild<QPushButton*>("checkBoxPunctuation")->isVisible());
                    REQUIRE(pPwdWidget->findChild<QLineEdit*>("editAdditionalChars")->isVisible());
                }
            }
        }

        WHEN("User unchecks the 'A-Z' button to get a password without upper-case letters")
        {
            auto* checkBoxUpper = pPwdWidget->findChild<QPushButton*>("checkBoxUpper");
            QTest::mouseClick(checkBoxUpper, Qt::MouseButton::LeftButton);

            THEN("New password is generated and has expected letters")
            {
                REQUIRE_FALSE(checkBoxUpper->isChecked());

                // May have a lower letter, a number, a special character, and must be 20 length.
                auto regex = R"(^(?=.*[^A-Z])(?=.*[a-z]*)(?=.*\d*)(?=.*[\(\)\[\]\{\}.,:;\"\'-\/\\_\\|!*+<=>?#\$%&@\^`~]*).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }
        }

        WHEN("User unchecks the 'a-z' button to get a password without lower-case letters")
        {
            auto* checkBoxLower = pPwdWidget->findChild<QPushButton*>("checkBoxLower");
            QTest::mouseClick(checkBoxLower, Qt::MouseButton::LeftButton);

            THEN("New password is generated and has expected letters")
            {
                REQUIRE_FALSE(checkBoxLower->isChecked());

                // May have an upper letter, a number, a special character, and must be 20 length.
                auto regex = R"(^(?=.*[A-Z]*)(?=.*[^a-z])(?=.*\d*)(?=.*[\(\)\[\]\{\}.,:;\"\'-\/\\_\\|!*+<=>?#\$%&@\^`~]*).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }
        }

        WHEN("User unchecks the '0-9' button to get a password without numbers")
        {
            auto* checkBoxNumbers = pPwdWidget->findChild<QPushButton*>("checkBoxNumbers");
            QTest::mouseClick(checkBoxNumbers, Qt::MouseButton::LeftButton);

            THEN("New password is generated and has expected letters")
            {
                REQUIRE_FALSE(checkBoxNumbers->isChecked());

                // May have a lower or upper letter, a special character, and must be 20 length.
                auto regex =
                    R"(^(?=.*[A-Z]*)(?=.*[a-z]*)(?=.*[^0-9])(?=.*[\(\)\[\]\{\}.,:;\"\'-\/\\_\\|!*+<=>?#\$%&@\^`~]*).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }
        }

        WHEN("User unchecks the '/ * + &...' button to get a password without special characters")
        {
            auto* checkBoxSpecialChars = pPwdWidget->findChild<QPushButton*>("checkBoxSpecialChars");
            QTest::mouseClick(checkBoxSpecialChars, Qt::MouseButton::LeftButton);

            THEN("New password is generated and has expected letters")
            {
                REQUIRE_FALSE(checkBoxSpecialChars->isChecked());

                // May have a lower or upper letter, a number, and must be 20 length but without special chars.
                auto regex = R"(^(?=.*[A-Z]*)(?=.*[a-z]*)(?=.*\d*)(?=.*[^\(\)\[\]\{\}.,:;\"\'-\/\\_\\|!*+<=>?#\$%&@\^`~]).{20}$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }
        }

        WHEN("User clicks the 'Extended ASCII' button to get a password with extended ASCII characters")
        {
            auto* checkBoxExtASCII = pPwdWidget->findChild<QPushButton*>("checkBoxExtASCII");
            QTest::mouseClick(checkBoxExtASCII, Qt::MouseButton::LeftButton);

            THEN("New password is generated and has expected letters")
            {
                REQUIRE(checkBoxExtASCII->isChecked());

                // May have a lower or upper letter, a number, an extended ASCII symbol.
                // Note, the length checking by {20} does not work with extended ASCII symbols but real length is 20.
                auto regex = R"(^(?=.*[A-Z]*)(?=.*[a-z]*)(?=.*\d*)(?=.*[\x80-\xFF]*).+$)";
                REQUIRE_THAT(pPwdGenWidget->getGeneratedPassword().toStdString(), Catch::Matchers::Matches(regex));
            }
        }

        WHEN("User enters own passwords")
        {
            // Data generator lets to reuse the same assertions of entropy and quality across different input passwords.
            struct TestData {
                QString password;
                QString expectedStrengthLabel;
            };

            auto data = GENERATE(
                // Empty password
                TestData{"", "Password Quality: Poor"},
                // Well-known password
                TestData{"hello", "Password Quality: Poor"},
                // Password composed of well-known words
                TestData{"helloworld", "Password Quality: Poor"},
                // Password composed of well-known words with number
                TestData{"password1", "Password Quality: Poor"},
                // Password out of small character space
                TestData{"D0g..................", "Password Quality: Poor"},
                // XKCD, easy substitutions
                TestData{"Tr0ub4dour&3", "Password Quality: Poor"},
                // XKCD, word generator
                TestData{"correcthorsebatterystaple", "Password Quality: Weak"},
                // Random characters, medium length
                TestData{"YQC3kbXbjC652dTDH", "Password Quality: Good"},
                // Random characters, long
                TestData{"Bs5ZFfthWzR8DGFEjaCM6bGqhmCT4km", "Password Quality: Excellent"},
                // Long password using Zxcvbn chunk estimation
                TestData{"quintet-tamper-kinswoman-humility-vengeful-haven-tastiness-aspire-widget-ipad-cussed-reaffirm-ladylike-"
                         "ashamed-anatomy-daybed-jam-swear-strudel-neatness-stalemate-unbundle-flavored-relation-emergency-underrate-"
                         "registry-getting-award-unveiled-unshaken-stagnate-cartridge-magnitude-ointment-hardener-enforced-scrubbed-"
                         "radial-fiddling-envelope-unpaved-moisture-unused-crawlers-quartered-crushed-kangaroo-tiptop-doily" ,
                         "Password Quality: Excellent"},
                // Longer password above Zxcvbn threshold
                TestData{"quintet-tamper-kinswoman-humility-vengeful-haven-tastiness-aspire-widget-ipad-cussed-reaffirm-ladylike-"
                         "ashamed-anatomy-daybed-jam-swear-strudel-neatness-stalemate-unbundle-flavored-relation-emergency-underrate-"
                         "registry-getting-award-unveiled-unshaken-stagnate-cartridge-magnitude-ointment-hardener-enforced-scrubbed-"
                         "radial-fiddling-envelope-unpaved-moisture-unused-crawlers-quartered-crushed-kangaroo-tiptop-doily-hefty-"
                         "untie-fidgeting-radiance-twilight-freebase-sulphuric-parrot-decree-monotype-nautical-pout-sip-geometric-"
                         "crunching-deviancy-festival-hacking-rage-unify-coronary-zigzagged-dwindle-possum-lilly-exhume-daringly-"
                         "barbell-rage-animate-lapel-emporium-renounce-justifier-relieving-gauze-arrive-alive-collected-immobile-"
                         "unleash-snowman-gift-expansion-marbles-requisite-excusable-flatness-displace-caloric-sensuous-moustache-"
                         "sensuous-capillary-aversion-contents-cadet-giggly-amenity-peddling-spotting-drier-mooned-rudder-peroxide-"
                         "posting-oppressor-scrabble-scorer-whomever-paprika-slapstick-said-spectacle-capture-debate-attire-emcee-"
                         "unfocused-sympathy-doily-election-ambulance-polish-subtype-grumbling-neon-stooge-reanalyze-rockfish-"
                         "disparate-decorated-washroom-threefold-muzzle-buckwheat-kerosene-swell-why-reprocess-correct-shady-"
                         "impatient-slit-banshee-scrubbed-dreadful-unlocking-urologist-hurried-citable-fragment-septic-lapped-"
                         "prankish-phantom-unpaved-moisture-unused-crawlers-quartered-crushed-kangaroo-lapel-emporium-renounce",
                         "Password Quality: Excellent"}
            );

            pPasswordEdit->setText(data.password);

            THEN("Entropy and password quality (strength) have expected values")
            {
                // Dynamically calculate entropy due to variances with zxcvbn wordlists
                PasswordHealth health(data.password);
                auto expectedEntropy = QString("Entropy: %1 bit").arg(QString::number(health.entropy(), 'f', 2));

                REQUIRE(findLabelText(pPwdGenWidget, "entropyLabel") == expectedEntropy);
                REQUIRE(findLabelText(pPwdGenWidget, "strengthLabel") == data.expectedStrengthLabel);
            }
        }

        // Close the widget after each test.
        escape(pPwdWidget);
        REQUIRE_FALSE(pPwdGenWidget->isVisible());
    }
}

SCENARIO_METHOD(FixtureWithDb, "Password Generator on new Entry", "[gui]")
{
    GIVEN("User creates a new Entry and opens a Password Generator from the password field by a shortcut")
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

                THEN("The Password Generator closed and a new passphrase is applied for the entry")
                {
                    //FIXME dmaslenko: "The emulated Apply click does not close the widget but applies the changes");
                    //REQUIRE_FALSE(pPassPhraseWidget->isVisible());
                    REQUIRE(actualPhase == actualPhase.toLower());
                    REQUIRE_THAT(actualPhase.toStdString(),
                                 Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
                }
            }
        }

        escape(pPassPhraseWidget);
    }
}
