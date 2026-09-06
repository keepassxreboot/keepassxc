/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#include "core/Database.h"
#include "core/Metadata.h"
#include <QCloseEvent>
#include <QDir>
#include <QInputDialog>
#include <QScopedValueRollback>
#include <QShortcut>
#include <QTimer>

#include "core/Config.h"
#include "core/PasswordHealth.h"
#include "core/Resources.h"
#include "gui/Clipboard.h"
#include "gui/FileDialog.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"
#include "gui/PasswordWidget.h"
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
    m_ui->buttonDeleteWordList->setIcon(icons()->icon("trash"));
    m_ui->buttonAddWordList->setIcon(icons()->icon("document-new"));
    m_ui->buttonClose->setShortcut(Qt::Key_Escape);

    // Add two shortcuts to save the form CTRL+Enter and CTRL+S
    auto shortcut = new QShortcut(Qt::CTRL + Qt::Key_Return, this);
    connect(shortcut, &QShortcut::activated, this, [this] { applyPassword(); });
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_S, this);
    connect(shortcut, &QShortcut::activated, this, [this] { applyPassword(); });

    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updateButtonsEnabled(QString)));
    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updatePasswordStrength()));
    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updatePasswordLengthLabel(QString)));
    connect(m_ui->buttonAdvancedMode, SIGNAL(toggled(bool)), SLOT(setAdvancedMode(bool)));
    connect(m_ui->buttonAddHex, SIGNAL(clicked()), SLOT(excludeHexChars()));
    connect(m_ui->editAdditionalChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->editExcludedChars, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(applyPassword()));
    connect(m_ui->buttonCopy, SIGNAL(clicked()), SLOT(copyPassword()));
    connect(m_ui->buttonGenerate, SIGNAL(clicked()), SLOT(regeneratePassword()));
    connect(m_ui->buttonDeleteWordList, SIGNAL(clicked()), SLOT(removeCustomWordList()));
    connect(m_ui->buttonAddWordList, SIGNAL(clicked()), SLOT(addWordList()));
    connect(m_ui->buttonClose, SIGNAL(clicked()), SIGNAL(closed()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(passwordLengthChanged(int)));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(passwordLengthChanged(int)));

    connect(m_ui->sliderWordCount, SIGNAL(valueChanged(int)), SLOT(passphraseLengthChanged(int)));
    connect(m_ui->spinBoxWordCount, SIGNAL(valueChanged(int)), SLOT(passphraseLengthChanged(int)));

    connect(m_ui->editWordSeparator, SIGNAL(textChanged(QString)), SLOT(updateGenerator()));
    connect(m_ui->comboBoxWordList, SIGNAL(currentIndexChanged(int)), SLOT(updateGenerator()));
    connect(m_ui->optionButtons, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(updateGenerator()));
    connect(m_ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(updateGenerator()));
    connect(m_ui->wordCaseComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateGenerator()));

    // set font size of password quality, characters, and entropy labels dynamically to 80% of
    // the default font size, but make it no smaller than 8pt
    QFont defaultFont;
    auto smallerSize = static_cast<int>(defaultFont.pointSize() * 0.8f);
    if (smallerSize >= 8) {
        defaultFont.setPointSize(smallerSize);
        m_ui->entropyLabel->setFont(defaultFont);
        m_ui->strengthLabel->setFont(defaultFont);
        m_ui->passwordLengthLabel->setFont(defaultFont);
    }

    // set default separator to Space
    m_ui->editWordSeparator->setText(PassphraseGenerator::DefaultSeparator);

    // add passphrase generator case options
    m_ui->wordCaseComboBox->addItem(tr("lower case"), PassphraseGenerator::LOWERCASE);
    m_ui->wordCaseComboBox->addItem(tr("UPPER CASE"), PassphraseGenerator::UPPERCASE);
    m_ui->wordCaseComboBox->addItem(tr("Title Case"), PassphraseGenerator::TITLECASE);
    m_ui->wordCaseComboBox->addItem(tr("MIXED case"), PassphraseGenerator::MIXEDCASE);

    // load system-wide wordlists
    QDir path(resources()->wordlistPath(""));
    for (const auto& fileName : path.entryList(QDir::Files)) {
        m_ui->comboBoxWordList->addItem(tr("(SYSTEM)") + " " + fileName, fileName);
    }

    m_firstCustomWordlistIndex = m_ui->comboBoxWordList->count();

    // load user-provided wordlists
    path = QDir(resources()->userWordlistPath(""));
    for (const auto& fileName : path.entryList(QDir::Files)) {
        m_ui->comboBoxWordList->addItem(fileName, path.absolutePath() + QDir::separator() + fileName);
    }

    // Set color of wordlist warning
    StateColorPalette statePalette;
    auto color = statePalette.color(StateColorPalette::ColorRole::False);
    m_ui->labelWordListWarning->setStyleSheet(QString("QLabel { color: %1; }").arg(color.name()));

    m_ui->profileContainer->hide();
    m_ui->profileWarningLabel->hide();
    connect(m_ui->profileComboBox, &QComboBox::currentIndexChanged, this, &PasswordGeneratorWidget::selectProfile);
    connect(m_ui->saveProfileButton, &QPushButton::clicked, this, &PasswordGeneratorWidget::saveProfile);
    connect(m_ui->deleteProfileButton, &QPushButton::clicked, this, &PasswordGeneratorWidget::removeProfile);
    connect(m_ui->defaultProfileButton, &QPushButton::clicked, this, &PasswordGeneratorWidget::setDefaultProfile);
    connect(m_ui->editNewPassword, &PasswordWidget::textChanged, this, [this] {
        if (!m_loadingSettings) {
            refreshProfiles();
        }
    });
    loadSettings();
}

PasswordGeneratorWidget::~PasswordGeneratorWidget() = default;

void PasswordGeneratorWidget::closeEvent(QCloseEvent* event)
{
    // Emits closed signal when clicking X from title bar
    emit closed();
    QWidget::closeEvent(event);
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
    QScopedValueRollback<bool> loading(m_loadingSettings, true);
    // Password config
    m_ui->checkBoxLower->setChecked(config()->get(Config::PasswordGenerator_LowerCase).toBool());
    m_ui->checkBoxUpper->setChecked(config()->get(Config::PasswordGenerator_UpperCase).toBool());
    m_ui->checkBoxNumbers->setChecked(config()->get(Config::PasswordGenerator_Numbers).toBool());
    m_ui->editAdditionalChars->setText(config()->get(Config::PasswordGenerator_AdditionalChars).toString());
    m_ui->editExcludedChars->setText(config()->get(Config::PasswordGenerator_ExcludedChars).toString());

    bool advanced = config()->get(Config::PasswordGenerator_AdvancedMode).toBool();
    if (advanced) {
        m_ui->checkBoxSpecialChars->setChecked(config()->get(Config::PasswordGenerator_Logograms).toBool());
    } else {
        m_ui->checkBoxSpecialChars->setChecked(config()->get(Config::PasswordGenerator_SpecialChars).toBool());
    }

    m_ui->checkBoxBraces->setChecked(config()->get(Config::PasswordGenerator_Braces).toBool());
    m_ui->checkBoxQuotes->setChecked(config()->get(Config::PasswordGenerator_Quotes).toBool());
    m_ui->checkBoxPunctuation->setChecked(config()->get(Config::PasswordGenerator_Punctuation).toBool());
    m_ui->checkBoxDashes->setChecked(config()->get(Config::PasswordGenerator_Dashes).toBool());
    m_ui->checkBoxMath->setChecked(config()->get(Config::PasswordGenerator_Math).toBool());

    m_ui->checkBoxExtASCII->setChecked(config()->get(Config::PasswordGenerator_EASCII).toBool());
    m_ui->checkBoxExcludeAlike->setChecked(config()->get(Config::PasswordGenerator_ExcludeAlike).toBool());
    m_ui->checkBoxEnsureEvery->setChecked(config()->get(Config::PasswordGenerator_EnsureEvery).toBool());
    m_ui->spinBoxLength->setValue(config()->get(Config::PasswordGenerator_Length).toInt());

    // Diceware config
    m_ui->spinBoxWordCount->setValue(config()->get(Config::PasswordGenerator_WordCount).toInt());
    m_ui->editWordSeparator->setText(config()->get(Config::PasswordGenerator_WordSeparator).toString());
    int i = m_ui->comboBoxWordList->findData(config()->get(Config::PasswordGenerator_WordList).toString());
    if (i > -1) {
        m_ui->comboBoxWordList->setCurrentIndex(i);
    }
    m_ui->wordCaseComboBox->setCurrentIndex(config()->get(Config::PasswordGenerator_WordCase).toInt());

    // Password or diceware?
    m_ui->tabWidget->setCurrentIndex(config()->get(Config::PasswordGenerator_Type).toInt());

    // Set advanced mode
    m_ui->buttonAdvancedMode->setChecked(advanced);
    setAdvancedMode(advanced);
    m_loadingSettings = false;
    updateGenerator();
}

void PasswordGeneratorWidget::saveSettings()
{
    if (m_loadingSettings || m_databaseSettings) {
        return;
    }
    // Password config
    config()->set(Config::PasswordGenerator_LowerCase, m_ui->checkBoxLower->isChecked());
    config()->set(Config::PasswordGenerator_UpperCase, m_ui->checkBoxUpper->isChecked());
    config()->set(Config::PasswordGenerator_Numbers, m_ui->checkBoxNumbers->isChecked());
    config()->set(Config::PasswordGenerator_EASCII, m_ui->checkBoxExtASCII->isChecked());

    config()->set(Config::PasswordGenerator_AdvancedMode, m_ui->buttonAdvancedMode->isChecked());
    if (m_ui->buttonAdvancedMode->isChecked()) {
        config()->set(Config::PasswordGenerator_Logograms, m_ui->checkBoxSpecialChars->isChecked());
    } else {
        config()->set(Config::PasswordGenerator_SpecialChars, m_ui->checkBoxSpecialChars->isChecked());
    }
    config()->set(Config::PasswordGenerator_Braces, m_ui->checkBoxBraces->isChecked());
    config()->set(Config::PasswordGenerator_Punctuation, m_ui->checkBoxPunctuation->isChecked());
    config()->set(Config::PasswordGenerator_Quotes, m_ui->checkBoxQuotes->isChecked());
    config()->set(Config::PasswordGenerator_Dashes, m_ui->checkBoxDashes->isChecked());
    config()->set(Config::PasswordGenerator_Math, m_ui->checkBoxMath->isChecked());

    config()->set(Config::PasswordGenerator_AdditionalChars, m_ui->editAdditionalChars->text());
    config()->set(Config::PasswordGenerator_ExcludedChars, m_ui->editExcludedChars->text());
    config()->set(Config::PasswordGenerator_ExcludeAlike, m_ui->checkBoxExcludeAlike->isChecked());
    config()->set(Config::PasswordGenerator_EnsureEvery, m_ui->checkBoxEnsureEvery->isChecked());
    config()->set(Config::PasswordGenerator_Length, m_ui->spinBoxLength->value());

    // Diceware config
    config()->set(Config::PasswordGenerator_WordCount, m_ui->spinBoxWordCount->value());
    config()->set(Config::PasswordGenerator_WordSeparator, m_ui->editWordSeparator->text());
    config()->set(Config::PasswordGenerator_WordList, m_ui->comboBoxWordList->currentData());
    config()->set(Config::PasswordGenerator_WordCase, m_ui->wordCaseComboBox->currentIndex());

    // Password or diceware?
    config()->set(Config::PasswordGenerator_Type, m_ui->tabWidget->currentIndex());
}

void PasswordGeneratorWidget::setPasswordLength(int length)
{
    if (length > 0) {
        m_ui->spinBoxLength->setValue(length);
    } else {
        m_ui->spinBoxLength->setValue(config()->get(Config::PasswordGenerator_Length).toInt());
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
    QScopedValueRollback<bool> loading(m_loadingSettings, true);
    QString password;
    if (!m_profileUnavailable) {
        if (m_ui->tabWidget->currentIndex() == Password) {
            if (m_passwordGenerator->isValid()) {
                password = m_passwordGenerator->generatePassword();
            }
        } else {
            password = m_dicewareGenerator->generatePassphrase();
        }
    }
    m_ui->editNewPassword->setText(password);
    m_ui->buttonGenerate->setEnabled(!password.isEmpty());
}

void PasswordGeneratorWidget::updateButtonsEnabled(const QString& password)
{
    if (!m_standalone) {
        m_ui->buttonApply->setEnabled(!password.isEmpty());
    }
    m_ui->buttonCopy->setEnabled(!password.isEmpty());
}

void PasswordGeneratorWidget::updatePasswordStrength()
{
    // Calculate the password / passphrase health
    PasswordHealth passwordHealth(0);
    if (m_ui->tabWidget->currentIndex() == Diceware) {
        passwordHealth.init(m_dicewareGenerator->estimateEntropy());
    } else {
        passwordHealth = PasswordHealth(m_ui->editNewPassword->text());
    }

    // Update the entropy text labels
    m_ui->entropyLabel->setText(tr("Entropy: %1 bit").arg(QString::number(passwordHealth.entropy(), 'f', 2)));
    m_ui->entropyProgressBar->setValue(std::min(int(passwordHealth.entropy()), m_ui->entropyProgressBar->maximum()));

    // Update the visual strength meter
    QString style = m_ui->entropyProgressBar->styleSheet();
    QRegularExpression re("(QProgressBar::chunk\\s*\\{.*?background-color:)[^;]+;",
                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    style.replace(re, "\\1 %1;");

    StateColorPalette statePalette;
    switch (passwordHealth.quality()) {
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

void PasswordGeneratorWidget::updatePasswordLengthLabel(const QString& password)
{
    m_ui->passwordLengthLabel->setText(tr("Characters: %1").arg(QString::number(password.length())));
}

void PasswordGeneratorWidget::applyPassword()
{
    if (m_ui->editNewPassword->text().isEmpty() || m_profileUnavailable) {
        return;
    }
    saveSettings();
    m_passwordGenerated = true;
    emit appliedProfile(selectedProfile());
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

bool PasswordGeneratorWidget::isPasswordGenerated() const
{
    return m_passwordGenerated;
}

void PasswordGeneratorWidget::removeCustomWordList()
{
    if (m_ui->comboBoxWordList->currentIndex() < m_firstCustomWordlistIndex) {
        return;
    }

    auto wordlist = m_ui->comboBoxWordList->currentText();
    auto result = MessageBox::question(this,
                                       tr("Confirm Remove Wordlist"),
                                       tr("Do you really want to remove the wordlist \"%1\"?").arg(wordlist),
                                       MessageBox::Remove | MessageBox::Cancel,
                                       MessageBox::Cancel);

    if (result == MessageBox::Remove) {
        QFile file(m_ui->comboBoxWordList->currentData().toString());
        if (file.exists() && !file.remove()) {
            MessageBox::critical(this, tr("Failed to delete wordlist"), file.errorString());
        }

        m_ui->comboBoxWordList->removeItem(m_ui->comboBoxWordList->currentIndex());
        updateGenerator();
    }
}

void PasswordGeneratorWidget::addWordList()
{
    auto filter = QString("%1 (*.txt *.asc *.wordlist);;%2 (*)").arg(tr("Wordlists"), tr("All files"));
    auto filePath = fileDialog()->getOpenFileName(this, tr("Select Custom Wordlist"), "", filter);
    if (filePath.isEmpty()) {
        return;
    }

    // create directory for user-specified wordlists, if necessary
    QDir destDir(resources()->userWordlistPath(""));
    destDir.mkpath(".");

    // check if destination wordlist already exists
    QString fileName = QFileInfo(filePath).fileName();
    QString destPath = destDir.absolutePath() + QDir::separator() + fileName;
    QFile dest(destPath);
    if (dest.exists()) {
        auto response = MessageBox::warning(this,
                                            tr("Overwrite Wordlist?"),
                                            tr("Wordlist \"%1\" already exists as a custom wordlist.\n"
                                               "Do you want to overwrite it?")
                                                .arg(fileName),
                                            MessageBox::Overwrite | MessageBox::Cancel,
                                            MessageBox::Cancel);
        if (response != MessageBox::Overwrite) {
            return;
        }
        if (!dest.remove()) {
            MessageBox::critical(this, tr("Failed to delete wordlist"), dest.errorString());
            return;
        }
    }

    // copy wordlist to destination path and add corresponding item to the combo box
    QFile file(filePath);
    if (!file.copy(destPath)) {
        MessageBox::critical(this, tr("Failed to add wordlist"), file.errorString());
        return;
    }

    auto index = m_ui->comboBoxWordList->findData(destPath);
    if (index == -1) {
        m_ui->comboBoxWordList->addItem(fileName, destPath);
        index = m_ui->comboBoxWordList->count() - 1;
    }
    m_ui->comboBoxWordList->setCurrentIndex(index);

    // update the password generator
    updateGenerator();
}

void PasswordGeneratorWidget::setAdvancedMode(bool advanced)
{
    if (!m_loadingSettings) {
        saveSettings();
    }

    if (advanced) {
        m_ui->checkBoxSpecialChars->setText("# $ % && @ ^ ` ~");
        m_ui->checkBoxSpecialChars->setToolTip(tr("Logograms"));
        if (!m_loadingSettings) {
            m_ui->checkBoxSpecialChars->setChecked(config()->get(Config::PasswordGenerator_Logograms).toBool());
        }
    } else {
        m_ui->checkBoxSpecialChars->setText("/ * + && …");
        m_ui->checkBoxSpecialChars->setToolTip(tr("Special Characters"));
        if (!m_loadingSettings) {
            m_ui->checkBoxSpecialChars->setChecked(config()->get(Config::PasswordGenerator_SpecialChars).toBool());
        }
    }

    m_ui->advancedContainer->setVisible(advanced);
    m_ui->checkBoxBraces->setVisible(advanced);
    m_ui->checkBoxPunctuation->setVisible(advanced);
    m_ui->checkBoxQuotes->setVisible(advanced);
    m_ui->checkBoxMath->setVisible(advanced);
    m_ui->checkBoxDashes->setVisible(advanced);

    updateGenerator();
    if (!m_standalone) {
        QTimer::singleShot(50, this, [this] { adjustSize(); });
    }
}

void PasswordGeneratorWidget::excludeHexChars()
{
    m_ui->editExcludedChars->setText("GHIJKLMNOPQRSTUVWXYZ");
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

PasswordGenerator::CharClasses PasswordGeneratorWidget::charClasses() const
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

PasswordGenerator::GeneratorFlags PasswordGeneratorWidget::generatorFlags() const
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
    if (m_loadingSettings) {
        return;
    }
    m_profileUnavailable = false;
    m_ui->profileWarningLabel->hide();
    refreshProfiles();
    if (m_ui->tabWidget->currentIndex() == Password) {
        auto classes = charClasses();
        auto flags = generatorFlags();

        m_passwordGenerator->setLength(m_ui->spinBoxLength->value());
        if (m_ui->buttonAdvancedMode->isChecked()) {
            m_passwordGenerator->setCharClasses(classes);
            m_passwordGenerator->setCustomCharacterSet(m_ui->editAdditionalChars->text());
            m_passwordGenerator->setExcludedCharacterSet(m_ui->editExcludedChars->text());
        } else {
            m_passwordGenerator->setCharClasses(classes);
            m_passwordGenerator->setCustomCharacterSet({});
            m_passwordGenerator->setExcludedCharacterSet({});
        }
        m_passwordGenerator->setFlags(flags);

        m_ui->buttonGenerate->setEnabled(m_passwordGenerator->isValid());
    } else {
        m_dicewareGenerator->setWordCase(
            static_cast<PassphraseGenerator::PassphraseWordCase>(m_ui->wordCaseComboBox->currentData().toInt()));

        m_dicewareGenerator->setWordCount(m_ui->spinBoxWordCount->value());
        auto path = m_ui->comboBoxWordList->currentData().toString();
        if (m_ui->comboBoxWordList->currentIndex() < m_firstCustomWordlistIndex) {
            path = resources()->wordlistPath(path);
            m_ui->buttonDeleteWordList->setEnabled(false);
        } else {
            m_ui->buttonDeleteWordList->setEnabled(true);
        }
        m_dicewareGenerator->setWordList(path);

        m_dicewareGenerator->setWordSeparator(m_ui->editWordSeparator->text());

        m_ui->labelWordListWarning->setVisible(!m_dicewareGenerator->isWordListValid());
    }

    regeneratePassword();
}

void PasswordGeneratorWidget::setDatabase(Database* database, const QUuid& profileId)
{
    disconnect(m_databaseConnection);
    disconnect(m_databaseDestroyedConnection);
    m_database = database;
    m_databaseSettings = false;
    m_profileUnavailable = false;
    m_ui->profileContainer->setVisible(database != nullptr);
    m_ui->profileWarningLabel->hide();
    refreshProfiles();
    if (!database) {
        return;
    }
    m_databaseDestroyedConnection =
        connect(database, &QObject::destroyed, this, &PasswordGeneratorWidget::clearProfileContext);
    m_databaseConnection = connect(database->metadata()->customData(),
                                   &CustomData::aboutToBeReset,
                                   this,
                                   &PasswordGeneratorWidget::clearProfileContext);
    const auto profile = profileId.isNull() ? database->defaultPasswordProfile() : database->passwordProfile(profileId);
    if (profile.isValid()) {
        loadProfile(profile);
    } else if (!profileId.isNull()) {
        m_profileUnavailable = true;
        m_ui->profileWarningLabel->setText(tr("The entry's password profile is unavailable in this database. "
                                              "Choose a profile or change the generator settings to continue."));
        m_ui->profileWarningLabel->show();
        m_ui->buttonGenerate->setEnabled(false);
        regeneratePassword();
    }
}

void PasswordGeneratorWidget::clearProfileContext()
{
    setDatabase(nullptr);
    loadSettings();
    m_ui->editNewPassword->clear();
    if (!m_standalone) {
        emit closed();
    }
}

QUuid PasswordGeneratorWidget::selectedProfile() const
{
    return m_ui->profileComboBox->currentData().toUuid();
}

void PasswordGeneratorWidget::refreshProfiles(const QUuid& selected)
{
    QSignalBlocker blocker(m_ui->profileComboBox);
    m_ui->profileComboBox->clear();
    m_ui->profileComboBox->addItem(tr("Custom settings"), QUuid());
    QUuid defaultId;
    if (m_database) {
        defaultId = m_database->defaultPasswordProfile().id();
        const auto profiles = m_database->passwordProfiles();
        for (const auto& profile : profiles) {
            const auto label = profile.id() == defaultId ? tr("%1 (default)").arg(profile.name()) : profile.name();
            m_ui->profileComboBox->addItem(label, profile.id());
        }
    }
    auto index = m_ui->profileComboBox->findData(selected);
    m_ui->profileComboBox->setCurrentIndex(qMax(0, index));
    m_ui->deleteProfileButton->setEnabled(index > 0);
    m_ui->defaultProfileButton->setEnabled(index > 0);
    m_ui->defaultProfileButton->setChecked(index > 0 && selected == defaultId);
}

PasswordProfile PasswordGeneratorWidget::currentProfile(const QString& name) const
{
    PasswordProfile profile(name);
    if (m_ui->tabWidget->currentIndex() == Password) {
        auto advanced = m_ui->buttonAdvancedMode->isChecked();
        auto flags = generatorFlags();
        if (advanced) {
            flags |= PasswordGenerator::AdvancedMode;
        }
        profile.setPasswordSettings(m_ui->spinBoxLength->value(),
                                    charClasses(),
                                    flags,
                                    advanced ? m_ui->editAdditionalChars->text() : QString(),
                                    advanced ? m_ui->editExcludedChars->text() : QString());
    } else {
        profile.setPassphraseSettings(
            m_ui->spinBoxWordCount->value(),
            static_cast<PassphraseGenerator::PassphraseWordCase>(m_ui->wordCaseComboBox->currentData().toInt()),
            m_ui->editWordSeparator->text(),
            m_ui->comboBoxWordList->currentData().toString());
    }
    return profile;
}

void PasswordGeneratorWidget::loadProfile(const PasswordProfile& profile)
{
    if (!profile.isValid()) {
        return;
    }
    m_databaseSettings = true;
    const auto settings = profile.toVariantMap();
    {
        QScopedValueRollback<bool> loading(m_loadingSettings, true);
        m_ui->tabWidget->setCurrentIndex(profile.type() == PasswordProfile::Password ? Password : Diceware);
        if (profile.type() == PasswordProfile::Password) {
            const auto classes = settings.value("charClasses").toInt();
            const auto flags = settings.value("generatorFlags").toInt();
            const bool advanced = flags != 0 || !settings.value("customCharacterSet").toString().isEmpty()
                                  || !settings.value("excludedCharacterSet").toString().isEmpty()
                                  || (classes & PasswordGenerator::SpecialCharacters) != 0;
            m_ui->buttonAdvancedMode->setChecked(advanced);
            setAdvancedMode(advanced);
            m_ui->checkBoxLower->setChecked(classes & PasswordGenerator::LowerLetters);
            m_ui->checkBoxUpper->setChecked(classes & PasswordGenerator::UpperLetters);
            m_ui->checkBoxNumbers->setChecked(classes & PasswordGenerator::Numbers);
            m_ui->checkBoxExtASCII->setChecked(classes & PasswordGenerator::EASCII);
            m_ui->checkBoxBraces->setChecked(classes & PasswordGenerator::Braces);
            m_ui->checkBoxPunctuation->setChecked(classes & PasswordGenerator::Punctuation);
            m_ui->checkBoxQuotes->setChecked(classes & PasswordGenerator::Quotes);
            m_ui->checkBoxDashes->setChecked(classes & PasswordGenerator::Dashes);
            m_ui->checkBoxMath->setChecked(classes & PasswordGenerator::Math);
            m_ui->checkBoxSpecialChars->setChecked(classes & PasswordGenerator::Logograms);
            m_ui->checkBoxExcludeAlike->setChecked(flags & PasswordGenerator::ExcludeLookAlike);
            m_ui->checkBoxEnsureEvery->setChecked(flags & PasswordGenerator::CharFromEveryGroup);
            m_ui->spinBoxLength->setValue(settings.value("passwordLength").toInt());
            m_ui->editAdditionalChars->setText(settings.value("customCharacterSet").toString());
            m_ui->editExcludedChars->setText(settings.value("excludedCharacterSet").toString());
        } else {
            m_ui->spinBoxWordCount->setValue(settings.value("passphraseWordCount").toInt());
            m_ui->wordCaseComboBox->setCurrentIndex(settings.value("wordCase").toInt());
            m_ui->editWordSeparator->setText(settings.value("wordSeparator").toString());
            m_ui->comboBoxWordList->setCurrentIndex(m_ui->comboBoxWordList->findData(settings.value("wordList")));
        }
    }
    updateGenerator();
    refreshProfiles(profile.id());
}

void PasswordGeneratorWidget::selectProfile(int index)
{
    if (m_loadingSettings || !m_database) {
        return;
    }
    if (index == 0) {
        loadSettings();
    } else {
        loadProfile(m_database->passwordProfile(selectedProfile()));
    }
}

void PasswordGeneratorWidget::saveProfile()
{
    if (!m_database || m_profileUnavailable) {
        return;
    }
    QPointer<PasswordGeneratorWidget> guard(this);
    bool ok = false;
    const auto previous = m_database->passwordProfile(selectedProfile());
    const auto name =
        QInputDialog::getText(
            this, tr("Save Password Profile"), tr("Profile name:"), QLineEdit::Normal, previous.name(), &ok)
            .trimmed();
    if (!guard || !ok || name.isEmpty() || !m_database) {
        return;
    }
    auto profile = currentProfile(name);
    if (!profile.isValid() || m_ui->editNewPassword->text().isEmpty()) {
        MessageBox::warning(
            this, tr("Invalid Profile"), tr("Choose valid generator settings before saving a profile."));
        return;
    }
    if (m_database->hasPasswordProfile(name)
        && MessageBox::question(this,
                                tr("Update Password Profile"),
                                tr("Replace profile \"%1\"? Entries using this profile will use the new settings the "
                                   "next time a password is generated.")
                                    .arg(name),
                                MessageBox::Yes | MessageBox::Cancel,
                                MessageBox::Cancel)
               != MessageBox::Yes) {
        return;
    }
    if (!guard || !m_database) {
        return;
    }
    if (!m_database->addPasswordProfile(profile)) {
        MessageBox::warning(
            this, tr("Cannot Save Profile"), tr("The database's password profiles could not be updated."));
        return;
    }
    m_databaseSettings = true;
    loadProfile(m_database->passwordProfile(name));
}

void PasswordGeneratorWidget::removeProfile()
{
    if (!m_database) {
        return;
    }
    const auto profile = m_database->passwordProfile(selectedProfile());
    if (!profile.isValid()) {
        return;
    }
    QPointer<PasswordGeneratorWidget> guard(this);
    if (MessageBox::question(this,
                             tr("Remove Password Profile"),
                             tr("Remove profile \"%1\" from this database? Entries using it will need another profile "
                                "when changing their passwords.")
                                 .arg(profile.name()),
                             MessageBox::Remove | MessageBox::Cancel,
                             MessageBox::Cancel)
            == MessageBox::Remove
        && guard && m_database) {
        m_database->removePasswordProfile(profile.name());
        refreshProfiles();
    }
}

void PasswordGeneratorWidget::setDefaultProfile()
{
    if (m_database) {
        const auto id = selectedProfile();
        m_database->setDefaultPasswordProfile(m_ui->defaultProfileButton->isChecked() ? id : QUuid());
        refreshProfiles(id);
    }
}
