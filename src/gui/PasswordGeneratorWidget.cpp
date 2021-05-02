/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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
#include <QShortcut>
#include <QTimer>

#include "core/Config.h"
#include "core/PasswordGenerator.h"
#include "core/PasswordGeneratorSettings.h"
#include "core/PasswordHealth.h"
#include "core/Resources.h"
#include "gui/Clipboard.h"
#include "gui/Icons.h"
#include "gui/styles/StateColorPalette.h"

PasswordGeneratorWidget::PasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_passwordGenerator(new PasswordGenerator())
    , m_dicewareGenerator(new PassphraseGenerator())
    , m_ui(new Ui::PasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    m_ui->buttonGenerate->setIcon(icons()->icon("refresh"));
    m_ui->buttonGenerate->setToolTip(
        tr("Regenerate password (%1)").arg(m_ui->buttonGenerate->shortcut().toString(QKeySequence::NativeText)));
    m_ui->buttonCopy->setIcon(icons()->icon("clipboard-text"));
    m_ui->buttonClose->setShortcut(Qt::Key_Escape);

    // Add two shortcuts to save the form CTRL+Enter and CTRL+S
    auto shortcut = new QShortcut(Qt::CTRL + Qt::Key_Return, this);
    connect(shortcut, &QShortcut::activated, this, [this] { applyPassword(); });
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_S, this);
    connect(shortcut, &QShortcut::activated, this, [this] { applyPassword(); });

    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updateButtonsEnabled(QString)));
    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updatePasswordStrength(QString)));
    connect(m_ui->buttonAdvancedMode, SIGNAL(toggled(bool)), SLOT(setAdvancedMode(bool)));
    connect(m_ui->buttonAddHex, SIGNAL(clicked()), SLOT(excludeHexChars()));
    connect(m_ui->editAdditionalChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->editExcludedChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(applyPassword()));
    connect(m_ui->buttonCopy, SIGNAL(clicked()), SLOT(copyPassword()));
    connect(m_ui->buttonGenerate, SIGNAL(clicked()), SLOT(regeneratePassword()));
    connect(m_ui->buttonClose, SIGNAL(clicked()), SIGNAL(closed()));

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

    connect(pwGenerator, SIGNAL(closed()), pwGenerator, SLOT(deleteLater()));

    pwGenerator->show();
    pwGenerator->raise();
    pwGenerator->activateWindow();
    pwGenerator->adjustSize();

    return pwGenerator;
}

void PasswordGeneratorWidget::loadSettings()
{
    // Password config
    m_ui->checkBoxLower->setChecked(passwordGeneratorSettings()->useLowercase());
    m_ui->checkBoxUpper->setChecked(passwordGeneratorSettings()->useUppercase());
    m_ui->checkBoxNumbers->setChecked(passwordGeneratorSettings()->useNumbers());
    m_ui->editAdditionalChars->setText(passwordGeneratorSettings()->additionalChars());
    m_ui->editExcludedChars->setText(passwordGeneratorSettings()->excludedChars());

    bool advanced = passwordGeneratorSettings()->advancedMode();
    if (advanced) {
        m_ui->checkBoxSpecialChars->setChecked(passwordGeneratorSettings()->useLogograms());
    } else {
        m_ui->checkBoxSpecialChars->setChecked(passwordGeneratorSettings()->useSpecial());
    }

    m_ui->checkBoxBraces->setChecked(passwordGeneratorSettings()->useBraces());
    m_ui->checkBoxQuotes->setChecked(passwordGeneratorSettings()->useQuotes());
    m_ui->checkBoxPunctuation->setChecked(passwordGeneratorSettings()->usePunctuation());
    m_ui->checkBoxDashes->setChecked(passwordGeneratorSettings()->useDashes());
    m_ui->checkBoxMath->setChecked(passwordGeneratorSettings()->useMath());

    m_ui->checkBoxExtASCII->setChecked(passwordGeneratorSettings()->useEASCII());
    m_ui->checkBoxExcludeAlike->setChecked(passwordGeneratorSettings()->excludeAlike());
    m_ui->checkBoxEnsureEvery->setChecked(passwordGeneratorSettings()->everyGroup());
    m_ui->spinBoxLength->setValue(passwordGeneratorSettings()->length());

    // Diceware config
    m_ui->spinBoxWordCount->setValue(passwordGeneratorSettings()->passPhraseWordCount());
    m_ui->editWordSeparator->setText(passwordGeneratorSettings()->passPhraseWordSeparator());
    m_ui->comboBoxWordList->setCurrentText(passwordGeneratorSettings()->passPhraseWordList());
    m_ui->wordCaseComboBox->setCurrentIndex(passwordGeneratorSettings()->passPhraseWordCase());

    // Password or diceware?
    m_ui->tabWidget->setCurrentIndex(passwordGeneratorSettings()->generatorType());

    // Set advanced mode
    m_ui->buttonAdvancedMode->setChecked(advanced);
    setAdvancedMode(advanced);
    updateGenerator();
}

void PasswordGeneratorWidget::saveSettings()
{
    // Password config
    passwordGeneratorSettings()->setUseLowercase(m_ui->checkBoxLower->isChecked());
    passwordGeneratorSettings()->setUseUppercase(m_ui->checkBoxUpper->isChecked());
    passwordGeneratorSettings()->setUseNumbers(m_ui->checkBoxNumbers->isChecked());
    passwordGeneratorSettings()->setUseEASCII(m_ui->checkBoxExtASCII->isChecked());

    passwordGeneratorSettings()->setAdvancedMode(m_ui->buttonAdvancedMode->isChecked());
    if (m_ui->buttonAdvancedMode->isChecked()) {
        passwordGeneratorSettings()->setUseLogograms(m_ui->checkBoxSpecialChars->isChecked());
    } else {
        passwordGeneratorSettings()->setUseSpecial(m_ui->checkBoxSpecialChars->isChecked());
    }
    passwordGeneratorSettings()->setUseBraces(m_ui->checkBoxBraces->isChecked());
    passwordGeneratorSettings()->setUsePunctuation(m_ui->checkBoxPunctuation->isChecked());
    passwordGeneratorSettings()->setUseQuotes(m_ui->checkBoxQuotes->isChecked());
    passwordGeneratorSettings()->setUseDashes(m_ui->checkBoxDashes->isChecked());
    passwordGeneratorSettings()->setUseMath(m_ui->checkBoxMath->isChecked());

    passwordGeneratorSettings()->setAdditionalChars(m_ui->editAdditionalChars->text());
    passwordGeneratorSettings()->setExcludedChars(m_ui->editExcludedChars->text());
    passwordGeneratorSettings()->setExcludeAlike(m_ui->checkBoxExcludeAlike->isChecked());
    passwordGeneratorSettings()->setEveryGroup(m_ui->checkBoxEnsureEvery->isChecked());
    passwordGeneratorSettings()->setLength(m_ui->spinBoxLength->value());

    // Diceware config
    passwordGeneratorSettings()->setPassPhraseWordCount(m_ui->spinBoxWordCount->value());
    passwordGeneratorSettings()->setPassPhraseWordSeparator(m_ui->editWordSeparator->text());
    passwordGeneratorSettings()->setPassPhraseWordList(m_ui->comboBoxWordList->currentText());
    passwordGeneratorSettings()->setPassPhraseWordCase(m_ui->wordCaseComboBox->currentIndex());

    // Password or diceware?
    passwordGeneratorSettings()->setGeneratorType(m_ui->tabWidget->currentIndex());
}

void PasswordGeneratorWidget::setPasswordLength(int length)
{
    if (length > 0) {
        m_ui->spinBoxLength->setValue(length);
    } else {
        m_ui->spinBoxLength->setValue(passwordGeneratorSettings()->length());
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

        m_ui->charactersInPassphraseLabel->setText(QString::number(password.length()));
    }

    m_ui->entropyLabel->setText(tr("Entropy: %1 bit").arg(QString::number(health.entropy(), 'f', 2)));

    m_ui->entropyProgressBar->setValue(std::min(int(health.entropy()), m_ui->entropyProgressBar->maximum()));

    colorStrengthIndicator(health);
}

void PasswordGeneratorWidget::applyPassword()
{
    saveSettings();
    emit appliedPassword(m_ui->editNewPassword->text());
    emit closed();
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

void PasswordGeneratorWidget::setAdvancedMode(bool advanced)
{
    saveSettings();

    if (advanced) {
        m_ui->checkBoxSpecialChars->setText("# $ % && @ ^ ` ~");
        m_ui->checkBoxSpecialChars->setToolTip(tr("Logograms"));
        m_ui->checkBoxSpecialChars->setChecked(passwordGeneratorSettings()->useLogograms());
    } else {
        m_ui->checkBoxSpecialChars->setText("/ * + && …");
        m_ui->checkBoxSpecialChars->setToolTip(tr("Special Characters"));
        m_ui->checkBoxSpecialChars->setChecked(passwordGeneratorSettings()->useSpecial());
    }

    m_ui->advancedContainer->setVisible(advanced);
    m_ui->checkBoxBraces->setVisible(advanced);
    m_ui->checkBoxPunctuation->setVisible(advanced);
    m_ui->checkBoxQuotes->setVisible(advanced);
    m_ui->checkBoxMath->setVisible(advanced);
    m_ui->checkBoxDashes->setVisible(advanced);

    if (!m_standalone) {
        QTimer::singleShot(50, this, [this] { adjustSize(); });
    }
}

void PasswordGeneratorWidget::excludeHexChars()
{
    m_ui->editExcludedChars->setText("GHIJKLMNOPQRSTUVWXYZghijklmnopqrstuvwxyz");
    m_ui->checkBoxNumbers->setChecked(true);
    m_ui->checkBoxUpper->setChecked(true);

    m_ui->checkBoxLower->setChecked(false);
    m_ui->checkBoxSpecialChars->setChecked(false);
    m_ui->checkBoxExtASCII->setChecked(false);
    m_ui->checkBoxPunctuation->setChecked(false);
    m_ui->checkBoxQuotes->setChecked(false);
    m_ui->checkBoxDashes->setChecked(false);
    m_ui->checkBoxMath->setChecked(false);
    m_ui->checkBoxBraces->setChecked(false);

    updateGenerator();
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

    if (m_ui->checkBoxLower->isChecked()) {
        classes |= PasswordGenerator::LowerLetters;
    }

    if (m_ui->checkBoxUpper->isChecked()) {
        classes |= PasswordGenerator::UpperLetters;
    }

    if (m_ui->checkBoxNumbers->isChecked()) {
        classes |= PasswordGenerator::Numbers;
    }

    if (m_ui->checkBoxExtASCII->isChecked()) {
        classes |= PasswordGenerator::EASCII;
    }

    if (!m_ui->buttonAdvancedMode->isChecked()) {
        if (m_ui->checkBoxSpecialChars->isChecked()) {
            classes |= PasswordGenerator::SpecialCharacters;
        }
    } else {
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

        if (m_ui->checkBoxSpecialChars->isChecked()) {
            classes |= PasswordGenerator::Logograms;
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
