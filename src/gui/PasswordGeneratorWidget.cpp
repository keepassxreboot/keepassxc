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
#include "core/PasswordGenerator.h"
#include "core/PasswordHealth.h"
#include "core/Resources.h"
#include "gui/Clipboard.h"
#include "gui/styles/StateColorPalette.h"

PasswordGeneratorWidget::PasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_passwordGenerator(new PasswordGenerator())
    , m_dicewareGenerator(new PassphraseGenerator())
    , m_ui(new Ui::PasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    m_ui->buttonGenerate->setIcon(resources()->icon("refresh"));
    m_ui->buttonGenerate->setToolTip(
        tr("Regenerate password (%1)").arg(m_ui->buttonGenerate->shortcut().toString(QKeySequence::NativeText)));
    m_ui->buttonCopy->setIcon(resources()->icon("clipboard-text"));
    m_ui->buttonClose->setShortcut(Qt::Key_Escape);

    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updateButtonsEnabled(QString)));
    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updatePasswordStrength(QString)));
    connect(m_ui->buttonAdvancedMode, SIGNAL(toggled(bool)), SLOT(setAdvancedMode(bool)));
    connect(m_ui->buttonAddHex, SIGNAL(clicked()), SLOT(excludeHexChars()));
    connect(m_ui->editAdditionalChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->editExcludedChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(applyPassword()));
    connect(m_ui->buttonCopy, SIGNAL(clicked()), SLOT(copyPassword()));
    connect(m_ui->buttonGenerate, SIGNAL(clicked()), SLOT(regeneratePassword()));
    connect(m_ui->buttonClose, SIGNAL(clicked()), SIGNAL(closePasswordGenerator()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(passwordLengthChanged(int)));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(passwordLengthChanged(int)));

    connect(m_ui->sliderWordCount, SIGNAL(valueChanged(int)), SLOT(passphraseLengthChanged(int)));
    connect(m_ui->spinBoxWordCount, SIGNAL(valueChanged(int)), SLOT(passphraseLengthChanged(int)));

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

    QDir path(resources()->wordlistPath(""));
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

PasswordGeneratorWidget* PasswordGeneratorWidget::popupGenerator(QWidget* parent)
{
    auto pwGenerator = new PasswordGeneratorWidget(parent);
    pwGenerator->setWindowModality(Qt::ApplicationModal);
    pwGenerator->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    pwGenerator->setStandaloneMode(false);

    connect(pwGenerator, SIGNAL(closePasswordGenerator()), pwGenerator, SLOT(deleteLater()));

    pwGenerator->show();
    pwGenerator->raise();
    pwGenerator->activateWindow();
    pwGenerator->adjustSize();

    return pwGenerator;
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
    m_ui->editAdditionalChars->setText(
        config()->get("generator/AdditionalChars", PasswordGenerator::DefaultAdditionalChars).toString());
    m_ui->editExcludedChars->setText(
        config()->get("generator/ExcludedChars", PasswordGenerator::DefaultExcludedChars).toString());

    m_ui->buttonAdvancedMode->setChecked(
        config()->get("generator/AdvancedMode", PasswordGenerator::DefaultAdvancedMode).toBool());
    setAdvancedMode(m_ui->buttonAdvancedMode->isChecked());

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
    config()->set("generator/AdvancedMode", m_ui->buttonAdvancedMode->isChecked());
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

void PasswordGeneratorWidget::setPasswordLength(int length)
{
    if (length > 0) {
        m_ui->spinBoxLength->setValue(length);
    } else {
        m_ui->spinBoxLength->setValue(config()->get("generator/Length", PasswordGenerator::DefaultLength).toInt());
    }
}

void PasswordGeneratorWidget::setStandaloneMode(bool standalone)
{
    m_standalone = standalone;
    if (standalone) {
        m_ui->buttonApply->setVisible(false);
        setPasswordVisible(true);
    } else {
        m_ui->buttonApply->setVisible(true);
    }
}

QString PasswordGeneratorWidget::getGeneratedPassword()
{
    return m_ui->editNewPassword->text();
}

void PasswordGeneratorWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape && m_standalone) {
        emit closePasswordGenerator();
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
    emit closePasswordGenerator();
}

void PasswordGeneratorWidget::copyPassword()
{
    clipboard()->setText(m_ui->editNewPassword->text());
}

void PasswordGeneratorWidget::passwordLengthChanged(int length)
{
    m_ui->spinBoxLength->blockSignals(true);
    m_ui->sliderLength->blockSignals(true);

    m_ui->spinBoxLength->setValue(length);
    m_ui->sliderLength->setValue(length);

    m_ui->spinBoxLength->blockSignals(false);
    m_ui->sliderLength->blockSignals(false);

    updateGenerator();
}

void PasswordGeneratorWidget::passphraseLengthChanged(int length)
{
    m_ui->spinBoxWordCount->blockSignals(true);
    m_ui->sliderWordCount->blockSignals(true);

    m_ui->spinBoxWordCount->setValue(length);
    m_ui->sliderWordCount->setValue(length);

    m_ui->spinBoxWordCount->blockSignals(false);
    m_ui->sliderWordCount->blockSignals(false);

    updateGenerator();
}

void PasswordGeneratorWidget::setPasswordVisible(bool visible)
{
    m_ui->editNewPassword->setShowPassword(visible);
}

bool PasswordGeneratorWidget::isPasswordVisible() const
{
    return m_ui->editNewPassword->isPasswordVisible();
}

void PasswordGeneratorWidget::setAdvancedMode(bool state)
{
    if (state) {
        m_ui->simpleBar->hide();
        m_ui->advancedContainer->show();
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
    } else {
        m_ui->simpleBar->show();
        m_ui->advancedContainer->hide();
        m_ui->checkBoxUpper->setChecked(m_ui->checkBoxUpperAdv->isChecked());
        m_ui->checkBoxLower->setChecked(m_ui->checkBoxLowerAdv->isChecked());
        m_ui->checkBoxNumbers->setChecked(m_ui->checkBoxNumbersAdv->isChecked());
        m_ui->checkBoxSpecialChars->setChecked(
            m_ui->checkBoxBraces->isChecked() | m_ui->checkBoxPunctuation->isChecked()
            | m_ui->checkBoxQuotes->isChecked() | m_ui->checkBoxMath->isChecked() | m_ui->checkBoxDashes->isChecked()
            | m_ui->checkBoxLogograms->isChecked());
        m_ui->checkBoxExtASCII->setChecked(m_ui->checkBoxExtASCIIAdv->isChecked());
    }

    QApplication::processEvents();
    adjustSize();
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

    StateColorPalette statePalette;
    switch (health.quality()) {
    case PasswordHealth::Quality::Bad:
    case PasswordHealth::Quality::Poor:
        m_ui->entropyProgressBar->setStyleSheet(
            style.arg(statePalette.color(StateColorPalette::HealthCritical).name()));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Poor", "Password quality")));
        break;

    case PasswordHealth::Quality::Weak:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(statePalette.color(StateColorPalette::HealthBad).name()));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Weak", "Password quality")));
        break;

    case PasswordHealth::Quality::Good:
        m_ui->entropyProgressBar->setStyleSheet(style.arg(statePalette.color(StateColorPalette::HealthOk).name()));
        m_ui->strengthLabel->setText(tr("Password Quality: %1").arg(tr("Good", "Password quality")));
        break;

    case PasswordHealth::Quality::Excellent:
        m_ui->entropyProgressBar->setStyleSheet(
            style.arg(statePalette.color(StateColorPalette::HealthExcellent).name()));
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

    if (m_ui->buttonAdvancedMode->isChecked()) {
        if (m_ui->checkBoxExcludeAlike->isChecked()) {
            flags |= PasswordGenerator::ExcludeLookAlike;
        }

        if (m_ui->checkBoxEnsureEvery->isChecked()) {
            flags |= PasswordGenerator::CharFromEveryGroup;
        }
    }

    return flags;
}

void PasswordGeneratorWidget::updateGenerator()
{
    if (m_ui->tabWidget->currentIndex() == Password) {
        auto classes = charClasses();
        auto flags = generatorFlags();

        int length = 0;
        if (flags.testFlag(PasswordGenerator::CharFromEveryGroup)) {
            if (classes.testFlag(PasswordGenerator::LowerLetters)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::UpperLetters)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Numbers)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Braces)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Punctuation)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Quotes)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Dashes)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Math)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::Logograms)) {
                ++length;
            }
            if (classes.testFlag(PasswordGenerator::EASCII)) {
                ++length;
            }
        }

        length = qMax(length, m_ui->spinBoxLength->value());
        m_passwordGenerator->setLength(length);
        m_passwordGenerator->setCharClasses(classes);
        m_passwordGenerator->setFlags(flags);
        if (m_ui->buttonAdvancedMode->isChecked()) {
            m_passwordGenerator->setAdditionalChars(m_ui->editAdditionalChars->text());
            m_passwordGenerator->setExcludedChars(m_ui->editExcludedChars->text());
        } else {
            m_passwordGenerator->setAdditionalChars("");
            m_passwordGenerator->setExcludedChars("");
        }

        if (m_passwordGenerator->isValid()) {
            m_ui->buttonGenerate->setEnabled(true);
        } else {
            m_ui->buttonGenerate->setEnabled(false);
        }
    } else {
        m_dicewareGenerator->setWordCase(
            static_cast<PassphraseGenerator::PassphraseWordCase>(m_ui->wordCaseComboBox->currentData().toInt()));

        m_dicewareGenerator->setWordCount(m_ui->spinBoxWordCount->value());
        if (!m_ui->comboBoxWordList->currentText().isEmpty()) {
            QString path = resources()->wordlistPath(m_ui->comboBoxWordList->currentText());
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
