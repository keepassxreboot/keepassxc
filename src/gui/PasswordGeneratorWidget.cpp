/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "PasswordGeneratorWidget.h"
#include "ui_PasswordGeneratorWidget.h"

#include <QDir>
#include <QKeyEvent>
#include <QLineEdit>

#include "core/Config.h"
#include "core/FilePath.h"
#include "core/PasswordGenerator.h"
#include "core/PasswordHealth.h"
#include "gui/Clipboard.h"
#include "gui/osutils/OSUtils.h"

PasswordGeneratorWidget::PasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingSpinBox(false)
    , m_passwordGenerator(new PasswordGenerator())
    , m_dicewareGenerator(new PassphraseGenerator())
    , m_ui(new Ui::PasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));

    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updateButtonsEnabled(QString)));
    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updatePasswordStrength(QString)));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(setPasswordVisible(bool)));
    connect(m_ui->buttonSimpleMode, SIGNAL(clicked()), SLOT(selectSimpleMode()));
    connect(m_ui->buttonAdvancedMode, SIGNAL(clicked()), SLOT(selectAdvancedMode()));
    connect(m_ui->buttonAddHex, SIGNAL(clicked()), SLOT(excludeHexChars()));
    connect(m_ui->editExcludedChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(applyPassword()));
    connect(m_ui->buttonCopy, SIGNAL(clicked()), SLOT(copyPassword()));
    connect(m_ui->buttonGenerate, SIGNAL(clicked()), SLOT(regeneratePassword()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(passwordSliderMoved()));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(passwordSpinBoxChanged()));

    connect(m_ui->sliderWordCount, SIGNAL(valueChanged(int)), SLOT(dicewareSliderMoved()));
    connect(m_ui->spinBoxWordCount, SIGNAL(valueChanged(int)), SLOT(dicewareSpinBoxChanged()));

    connect(m_ui->editWordSeparator, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->comboBoxWordList, SIGNAL(currentIndexChanged(int)), SLOT(updateGenerator()));
    connect(m_ui->optionButtons, SIGNAL(buttonClicked(int)), SLOT(updateGenerator()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(updateGenerator()));
    connect(m_ui->wordCaseComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateGenerator()));

    // set font size of password quality and entropy labels dynamically to 80% of
    // the default font size, but make it no smaller than 8pt
    QFont defaultFont;
    int smallerSize = static_cast<int>(defaultFont.pointSize() * 0.8f);
    if (smallerSize >= 8) {
        defaultFont.setPointSize(smallerSize);
        m_ui->entropyLabel->setFont(defaultFont);
        m_ui->strengthLabel->setFont(defaultFont);
    }

    // set default separator to Space
    m_ui->editWordSeparator->setText(PassphraseGenerator::DefaultSeparator);

    // add passphrase generator case options
    m_ui->wordCaseComboBox->addItem(tr("lower case"), PassphraseGenerator::LOWERCASE);
    m_ui->wordCaseComboBox->addItem(tr("UPPER CASE"), PassphraseGenerator::UPPERCASE);
    m_ui->wordCaseComboBox->addItem(tr("Title Case"), PassphraseGenerator::TITLECASE);

    QDir path(filePath()->wordlistPath(""));
    QStringList files = path.entryList(QDir::Files);
    m_ui->comboBoxWordList->addItems(files);
    if (files.size() > 1) {
        m_ui->comboBoxWordList->setVisible(true);
        m_ui->labelWordList->setVisible(true);
    } else {
        m_ui->comboBoxWordList->setVisible(false);
        m_ui->labelWordList->setVisible(false);
    }

    loadSettings();
}

PasswordGeneratorWidget::~PasswordGeneratorWidget()
{
}

void PasswordGeneratorWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    reset();
}

void PasswordGeneratorWidget::loadSettings()
{
    // Password config
    m_ui->checkBoxLower->setChecked(config()->get("generator/LowerCase", PasswordGenerator::DefaultLower).toBool());
    m_ui->checkBoxLowerAdv->setChecked(config()->get("generator/LowerCase", PasswordGenerator::DefaultLower).toBool());
    m_ui->checkBoxUpper->setChecked(config()->get("generator/UpperCase", PasswordGenerator::DefaultUpper).toBool());
    m_ui->checkBoxUpperAdv->setChecked(config()->get("generator/UpperCase", PasswordGenerator::DefaultUpper).toBool());
    m_ui->checkBoxNumbers->setChecked(config()->get("generator/Numbers", PasswordGenerator::DefaultNumbers).toBool());
    m_ui->checkBoxSpecialChars->setChecked(
        config()->get("generator/SpecialChars", PasswordGenerator::DefaultSpecial).toBool());
    m_ui->checkBoxNumbersAdv->setChecked(
        config()->get("generator/Numbers", PasswordGenerator::DefaultNumbers).toBool());
    m_ui->advancedBar->setVisible(
        config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool());
    m_ui->excludedChars->setVisible(
        config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool());
    m_ui->checkBoxExcludeAlike->setVisible(
        config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool());
    m_ui->checkBoxEnsureEvery->setVisible(
        config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool());
    m_ui->editExcludedChars->setText(
        config()->get("generator/ExcludedChars", PasswordGenerator::DefaultExcludedChars).toString());

    m_ui->simpleBar->setVisible(
        !(config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool()));
    m_ui->checkBoxBraces->setChecked(config()->get("generator/Braces", PasswordGenerator::DefaultBraces).toBool());
    m_ui->checkBoxQuotes->setChecked(config()->get("generator/Quotes", PasswordGenerator::DefaultQuotes).toBool());
    m_ui->checkBoxPunctuation->setChecked(
        config()->get("generator/Punctuation", PasswordGenerator::DefaultPunctuation).toBool());
    m_ui->checkBoxDashes->setChecked(config()->get("generator/Dashes", PasswordGenerator::DefaultDashes).toBool());
    m_ui->checkBoxMath->setChecked(config()->get("generator/Math", PasswordGenerator::DefaultMath).toBool());
    m_ui->checkBoxLogograms->setChecked(
        config()->get("generator/Logograms", PasswordGenerator::DefaultLogograms).toBool());
    m_ui->checkBoxExtASCII->setChecked(config()->get("generator/EASCII", PasswordGenerator::DefaultEASCII).toBool());
    m_ui->checkBoxExtASCIIAdv->setChecked(config()->get("generator/EASCII", PasswordGenerator::DefaultEASCII).toBool());
    m_ui->checkBoxExcludeAlike->setChecked(
        config()->get("generator/ExcludeAlike", PasswordGenerator::DefaultLookAlike).toBool());
    m_ui->checkBoxEnsureEvery->setChecked(
        config()->get("generator/EnsureEvery", PasswordGenerator::DefaultFromEveryGroup).toBool());
    m_ui->spinBoxLength->setValue(config()->get("generator/Length", PasswordGenerator::DefaultLength).toInt());

    // Diceware config
    m_ui->spinBoxWordCount->setValue(
        config()->get("generator/WordCount", PassphraseGenerator::DefaultWordCount).toInt());
    m_ui->editWordSeparator->setText(
        config()->get("generator/WordSeparator", PassphraseGenerator::DefaultSeparator).toString());
    m_ui->comboBoxWordList->setCurrentText(
        config()->get("generator/WordList", PassphraseGenerator::DefaultWordList).toString());
    m_ui->wordCaseComboBox->setCurrentIndex(config()->get("generator/WordCase", 0).toInt());

    // Password or diceware?
    m_ui->tabWidget->setCurrentIndex(config()->get("generator/Type", 0).toInt());
}

void PasswordGeneratorWidget::saveSettings()
{
    // Password config
    if (m_ui->simpleBar->isVisible()) {
        config()->set("generator/LowerCase", m_ui->checkBoxLower->isChecked());
        config()->set("generator/UpperCase", m_ui->checkBoxUpper->isChecked());
        config()->set("generator/Numbers", m_ui->checkBoxNumbers->isChecked());
        config()->set("generator/EASCII", m_ui->checkBoxExtASCII->isChecked());
    } else {
        config()->set("generator/LowerCase", m_ui->checkBoxLowerAdv->isChecked());
        config()->set("generator/UpperCase", m_ui->checkBoxUpperAdv->isChecked());
        config()->set("generator/Numbers", m_ui->checkBoxNumbersAdv->isChecked());
        config()->set("generator/EASCII", m_ui->checkBoxExtASCIIAdv->isChecked());
    }
    config()->set("generator/AdvancedMode", m_ui->advancedBar->isVisible());
    config()->set("generator/SpecialChars", m_ui->checkBoxSpecialChars->isChecked());
    config()->set("generator/Braces", m_ui->checkBoxBraces->isChecked());
    config()->set("generator/Punctuation", m_ui->checkBoxPunctuation->isChecked());
    config()->set("generator/Quotes", m_ui->checkBoxQuotes->isChecked());
    config()->set("generator/Dashes", m_ui->checkBoxDashes->isChecked());
    config()->set("generator/Math", m_ui->checkBoxMath->isChecked());
    config()->set("generator/Logograms", m_ui->checkBoxLogograms->isChecked());
    config()->set("generator/ExcludedChars", m_ui->editExcludedChars->text());
    config()->set("generator/ExcludeAlike", m_ui->checkBoxExcludeAlike->isChecked());
    config()->set("generator/EnsureEvery", m_ui->checkBoxEnsureEvery->isChecked());
    config()->set("generator/Length", m_ui->spinBoxLength->value());

    // Diceware config
    config()->set("generator/WordCount", m_ui->spinBoxWordCount->value());
    config()->set("generator/WordSeparator", m_ui->editWordSeparator->text());
    config()->set("generator/WordList", m_ui->comboBoxWordList->currentText());
    config()->set("generator/WordCase", m_ui->wordCaseComboBox->currentIndex());

    // Password or diceware?
    config()->set("generator/Type", m_ui->tabWidget->currentIndex());
}

void PasswordGeneratorWidget::reset(int length)
{
    m_ui->editNewPassword->setText("");
    if (length > 0) {
        m_ui->spinBoxLength->setValue(length);
    } else {
        m_ui->spinBoxLength->setValue(config()->get("generator/Length", PasswordGenerator::DefaultLength).toInt());
    }

    setStandaloneMode(false);
    setPasswordVisible(config()->get("security/passwordscleartext").toBool());
    updateGenerator();
}

void PasswordGeneratorWidget::setStandaloneMode(bool standalone)
{
    m_standalone = standalone;
    if (standalone) {
        m_ui->buttonApply->setText(tr("Close"));
        setPasswordVisible(true);
    } else {
        m_ui->buttonApply->setText(tr("Accept"));
    }
}

QString PasswordGeneratorWidget::getGeneratedPassword()
{
    return m_ui->editNewPassword->text();
}

void PasswordGeneratorWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_standalone) {
        emit dialogTerminated();
    } else {
        e->ignore();
    }
}

void PasswordGeneratorWidget::regeneratePassword()
{
    if (m_ui->tabWidget->currentIndex() == Password) {
        if (m_passwordGenerator->isValid()) {
            QString password = m_passwordGenerator->generatePassword();
            m_ui->editNewPassword->setText(password);
            updatePasswordStrength(password);
        }
    } else {
        if (m_dicewareGenerator->isValid()) {
            QString password = m_dicewareGenerator->generatePassphrase();
            m_ui->editNewPassword->setText(password);
            updatePasswordStrength(password);
        }
    }
}

void PasswordGeneratorWidget::updateButtonsEnabled(const QString& password)
{
    if (!m_standalone) {
        m_ui->buttonApply->setEnabled(!password.isEmpty());
    }
    m_ui->buttonCopy->setEnabled(!password.isEmpty());
}

void PasswordGeneratorWidget::updatePasswordStrength(const QString& password)
{
    PasswordHealth health(password);
    if (m_ui->tabWidget->currentIndex() == Diceware) {
        // Diceware estimates entropy differently
        health = PasswordHealth(m_dicewareGenerator->estimateEntropy());
    }

    m_ui->entropyLabel->setText(tr("Entropy: %1 bit").arg(QString::number(health.entropy(), 'f', 2)));

    m_ui->entropyProgressBar->setValue(std::min(int(health.entropy()), m_ui->entropyProgressBar->maximum()));

    colorStrengthIndicator(health);
}

void PasswordGeneratorWidget::applyPassword()
{
    saveSettings();
    emit appliedPassword(m_ui->editNewPassword->text());
    emit dialogTerminated();
}

void PasswordGeneratorWidget::copyPassword()
{
    clipboard()->setText(m_ui->editNewPassword->text());
}

void PasswordGeneratorWidget::passwordSliderMoved()
{
    if (m_updatingSpinBox) {
        return;
    }

    m_ui->spinBoxLength->setValue(m_ui->sliderLength->value());

    updateGenerator();
}

void PasswordGeneratorWidget::passwordSpinBoxChanged()
{
    if (m_updatingSpinBox) {
        return;
    }

    // Interlock so that we don't update twice - this causes issues as the spinbox can go higher than slider
    m_updatingSpinBox = true;

    m_ui->sliderLength->setValue(m_ui->spinBoxLength->value());

    m_updatingSpinBox = false;

    updateGenerator();
}

void PasswordGeneratorWidget::dicewareSliderMoved()
{
    m_ui->spinBoxWordCount->setValue(m_ui->sliderWordCount->value());

    updateGenerator();
}

void PasswordGeneratorWidget::dicewareSpinBoxChanged()
{
    m_ui->sliderWordCount->setValue(m_ui->spinBoxWordCount->value());

    updateGenerator();
}

void PasswordGeneratorWidget::setPasswordVisible(bool visible)
{
    m_ui->editNewPassword->setShowPassword(visible);
    bool blockSignals = m_ui->togglePasswordButton->blockSignals(true);
    m_ui->togglePasswordButton->setChecked(visible);
    m_ui->togglePasswordButton->blockSignals(blockSignals);
}

bool PasswordGeneratorWidget::isPasswordVisible() const
{
    return m_ui->togglePasswordButton->isChecked();
}

void PasswordGeneratorWidget::selectSimpleMode()
{
    m_ui->advancedBar->hide();
    m_ui->excludedChars->hide();
    m_ui->checkBoxExcludeAlike->hide();
    m_ui->checkBoxEnsureEvery->hide();
    m_ui->checkBoxUpper->setChecked(m_ui->checkBoxUpperAdv->isChecked());
    m_ui->checkBoxLower->setChecked(m_ui->checkBoxLowerAdv->isChecked());
    m_ui->checkBoxNumbers->setChecked(m_ui->checkBoxNumbersAdv->isChecked());
    m_ui->checkBoxSpecialChars->setChecked(m_ui->checkBoxBraces->isChecked() | m_ui->checkBoxPunctuation->isChecked()
                                           | m_ui->checkBoxQuotes->isChecked() | m_ui->checkBoxMath->isChecked()
                                           | m_ui->checkBoxDashes->isChecked() | m_ui->checkBoxLogograms->isChecked());
    m_ui->checkBoxExtASCII->setChecked(m_ui->checkBoxExtASCIIAdv->isChecked());
    m_ui->simpleBar->show();
}

void PasswordGeneratorWidget::selectAdvancedMode()
{
    m_ui->simpleBar->hide();
    m_ui->checkBoxUpperAdv->setChecked(m_ui->checkBoxUpper->isChecked());
    m_ui->checkBoxLowerAdv->setChecked(m_ui->checkBoxLower->isChecked());
    m_ui->checkBoxNumbersAdv->setChecked(m_ui->checkBoxNumbers->isChecked());
    m_ui->checkBoxBraces->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxPunctuation->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxQuotes->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxMath->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxDashes->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxLogograms->setChecked(m_ui->checkBoxSpecialChars->isChecked());
    m_ui->checkBoxExtASCIIAdv->setChecked(m_ui->checkBoxExtASCII->isChecked());
    m_ui->advancedBar->show();
    m_ui->excludedChars->show();
    m_ui->checkBoxExcludeAlike->show();
    m_ui->checkBoxEnsureEvery->show();
}

void PasswordGeneratorWidget::excludeHexChars()
{
    m_ui->editExcludedChars->setText("GHIJKLMNOPQRSTUVWXYZghijklmnopqrstuvwxyz");
}

void PasswordGeneratorWidget::colorStrengthIndicator(const PasswordHealth& health)
{
    // Take the existing stylesheet and convert the text and background color to arguments
    QString style = m_ui->entropyProgressBar->styleSheet();
    QRegularExpression re("(QProgressBar::chunk\\s*\\{.*?background-color:)[^;]+;",
                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    style.replace(re, "\\1 %1;");

    // Set the color and background based on entropy
    QList<QString> qualityColors;
    if (osUtils->isDarkMode()) {
        qualityColors << QStringLiteral("#C43F31") << QStringLiteral("#DB9837") << QStringLiteral("#608A22")
                      << QStringLiteral("#1F8023");
    } else {
        qualityColors << QStringLiteral("#C43F31") << QStringLiteral("#E09932") << QStringLiteral("#5EA10E")
                      << QStringLiteral("#118f17");
    }
    switch (health.quality()) {
    case PasswordHealth::Quality::Bad:
    case PasswordHealth::Quality::Poor:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(qualityColors[0]));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Poor", "Password quality")));
        break;

    case PasswordHealth::Quality::Weak:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(qualityColors[1]));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Weak", "Password quality")));
        break;

    case PasswordHealth::Quality::Good:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(qualityColors[2]));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Good", "Password quality")));
        break;

    case PasswordHealth::Quality::Excellent:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(qualityColors[3]));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Excellent", "Password quality")));
        break;
    }
}

PasswordGenerator::CharClasses PasswordGeneratorWidget::charClasses()
{
    PasswordGenerator::CharClasses classes;

    if (m_ui->simpleBar->isVisible()) {
        if (m_ui->checkBoxLower->isChecked()) {
            classes |= PasswordGenerator::LowerLetters;
        }

        if (m_ui->checkBoxUpper->isChecked()) {
            classes |= PasswordGenerator::UpperLetters;
        }

        if (m_ui->checkBoxNumbers->isChecked()) {
            classes |= PasswordGenerator::Numbers;
        }

        if (m_ui->checkBoxSpecialChars->isChecked()) {
            classes |= PasswordGenerator::SpecialCharacters;
        }

        if (m_ui->checkBoxExtASCII->isChecked()) {
            classes |= PasswordGenerator::EASCII;
        }
    } else {
        if (m_ui->checkBoxLowerAdv->isChecked()) {
            classes |= PasswordGenerator::LowerLetters;
        }

        if (m_ui->checkBoxUpperAdv->isChecked()) {
            classes |= PasswordGenerator::UpperLetters;
        }

        if (m_ui->checkBoxNumbersAdv->isChecked()) {
            classes |= PasswordGenerator::Numbers;
        }

        if (m_ui->checkBoxBraces->isChecked()) {
            classes |= PasswordGenerator::Braces;
        }

        if (m_ui->checkBoxPunctuation->isChecked()) {
            classes |= PasswordGenerator::Punctuation;
        }

        if (m_ui->checkBoxQuotes->isChecked()) {
            classes |= PasswordGenerator::Quotes;
        }

        if (m_ui->checkBoxDashes->isChecked()) {
            classes |= PasswordGenerator::Dashes;
        }

        if (m_ui->checkBoxMath->isChecked()) {
            classes |= PasswordGenerator::Math;
        }

        if (m_ui->checkBoxLogograms->isChecked()) {
            classes |= PasswordGenerator::Logograms;
        }

        if (m_ui->checkBoxExtASCIIAdv->isChecked()) {
            classes |= PasswordGenerator::EASCII;
        }
    }

    return classes;
}

PasswordGenerator::GeneratorFlags PasswordGeneratorWidget::generatorFlags()
{
    PasswordGenerator::GeneratorFlags flags;

    if (m_ui->checkBoxExcludeAlike->isChecked()) {
        flags |= PasswordGenerator::ExcludeLookAlike;
    }

    if (m_ui->checkBoxEnsureEvery->isChecked()) {
        flags |= PasswordGenerator::CharFromEveryGroup;
    }

    return flags;
}

void PasswordGeneratorWidget::updateGenerator()
{
    if (m_ui->tabWidget->currentIndex() == Password) {
        PasswordGenerator::CharClasses classes = charClasses();
        PasswordGenerator::GeneratorFlags flags = generatorFlags();

        int minLength = 0;
        if (flags.testFlag(PasswordGenerator::CharFromEveryGroup)) {
            if (classes.testFlag(PasswordGenerator::LowerLetters)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::UpperLetters)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Numbers)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Braces)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Punctuation)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Quotes)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Dashes)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Math)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::Logograms)) {
                minLength++;
            }
            if (classes.testFlag(PasswordGenerator::EASCII)) {
                minLength++;
            }
        }
        minLength = qMax(minLength, 1);

        if (m_ui->spinBoxLength->value() < minLength) {
            m_updatingSpinBox = true;
            m_ui->spinBoxLength->setValue(minLength);
            m_ui->sliderLength->setValue(minLength);
            m_updatingSpinBox = false;
        }

        m_ui->spinBoxLength->setMinimum(minLength);
        m_ui->sliderLength->setMinimum(minLength);

        m_passwordGenerator->setLength(m_ui->spinBoxLength->value());
        m_passwordGenerator->setCharClasses(classes);
        if (m_ui->simpleBar->isVisible()) {
            m_passwordGenerator->setExcludedChars("");
        } else {
            m_passwordGenerator->setExcludedChars(m_ui->editExcludedChars->text());
        }
        m_passwordGenerator->setFlags(flags);

        if (m_passwordGenerator->isValid()) {
            m_ui->buttonGenerate->setEnabled(true);
        } else {
            m_ui->buttonGenerate->setEnabled(false);
        }
    } else {
        int minWordCount = 1;

        if (m_ui->spinBoxWordCount->value() < minWordCount) {
            m_updatingSpinBox = true;
            m_ui->spinBoxWordCount->setValue(minWordCount);
            m_ui->sliderWordCount->setValue(minWordCount);
            m_updatingSpinBox = false;
        }

        m_dicewareGenerator->setWordCase(
            static_cast<PassphraseGenerator::PassphraseWordCase>(m_ui->wordCaseComboBox->currentData().toInt()));

        m_ui->spinBoxWordCount->setMinimum(minWordCount);
        m_ui->sliderWordCount->setMinimum(minWordCount);

        m_dicewareGenerator->setWordCount(m_ui->spinBoxWordCount->value());
        if (!m_ui->comboBoxWordList->currentText().isEmpty()) {
            QString path = filePath()->wordlistPath(m_ui->comboBoxWordList->currentText());
            m_dicewareGenerator->setWordList(path);
        }
        m_dicewareGenerator->setWordSeparator(m_ui->editWordSeparator->text());

        if (m_dicewareGenerator->isValid()) {
            m_ui->buttonGenerate->setEnabled(true);
        } else {
            m_ui->buttonGenerate->setEnabled(false);
        }
    }

    regeneratePassword();
}
