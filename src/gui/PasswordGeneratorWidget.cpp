/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include <QLineEdit>

#include "core/Config.h"
#include "core/PasswordGenerator.h"
#include "core/FilePath.h"

PasswordGeneratorWidget::PasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingSpinBox(false)
    , m_generator(new PasswordGenerator())
    , m_ui(new Ui::PasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));

    connect(m_ui->editNewPassword->lineEdit(), SIGNAL(textChanged(QString)), SLOT(updateApplyEnabled(QString)));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), m_ui->editNewPassword, SLOT(setEcho(bool)));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(emitNewPassword()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(saveSettings()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(sliderMoved()));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(spinBoxChanged()));

    connect(m_ui->optionButtons, SIGNAL(buttonClicked(int)), SLOT(updateGenerator()));

    m_ui->editNewPassword->setGenerator(m_generator.data());

    loadSettings();
    reset();
}

PasswordGeneratorWidget::~PasswordGeneratorWidget()
{
}

void PasswordGeneratorWidget::loadSettings()
{
    m_ui->checkBoxLower->setChecked(config()->get("generator/LowerCase", true).toBool());
    m_ui->checkBoxUpper->setChecked(config()->get("generator/UpperCase", true).toBool());
    m_ui->checkBoxNumbers->setChecked(config()->get("generator/Numbers", true).toBool());
    m_ui->checkBoxSpecialChars->setChecked(config()->get("generator/SpecialChars", false).toBool());

    m_ui->checkBoxExcludeAlike->setChecked(config()->get("generator/ExcludeAlike", true).toBool());
    m_ui->checkBoxEnsureEvery->setChecked(config()->get("generator/EnsureEvery", true).toBool());

    m_ui->spinBoxLength->setValue(config()->get("generator/Length", 16).toInt());
}

void PasswordGeneratorWidget::saveSettings()
{
    config()->set("generator/LowerCase", m_ui->checkBoxLower->isChecked());
    config()->set("generator/UpperCase", m_ui->checkBoxUpper->isChecked());
    config()->set("generator/Numbers", m_ui->checkBoxNumbers->isChecked());
    config()->set("generator/SpecialChars", m_ui->checkBoxSpecialChars->isChecked());

    config()->set("generator/ExcludeAlike", m_ui->checkBoxExcludeAlike->isChecked());
    config()->set("generator/EnsureEvery", m_ui->checkBoxEnsureEvery->isChecked());

    config()->set("generator/Length", m_ui->spinBoxLength->value());
}

void PasswordGeneratorWidget::reset()
{
    m_ui->editNewPassword->lineEdit()->setText("");
    m_ui->togglePasswordButton->setChecked(config()->get("security/passwordscleartext").toBool());

    updateGenerator();
}

void PasswordGeneratorWidget::regeneratePassword()
{
    if (m_generator->isValid()) {
        QString password = m_generator->generatePassword();
        m_ui->editNewPassword->setEditText(password);
    }
}

void PasswordGeneratorWidget::updateApplyEnabled(const QString& password)
{
    m_ui->buttonApply->setEnabled(!password.isEmpty());
}

void PasswordGeneratorWidget::emitNewPassword()
{
    Q_EMIT newPassword(m_ui->editNewPassword->lineEdit()->text());
}

void PasswordGeneratorWidget::sliderMoved()
{
    if (m_updatingSpinBox) {
        return;
    }

    m_ui->spinBoxLength->setValue(m_ui->sliderLength->value());

    updateGenerator();
}

void PasswordGeneratorWidget::spinBoxChanged()
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

    if (m_ui->checkBoxSpecialChars->isChecked()) {
        classes |= PasswordGenerator::SpecialCharacters;
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
        if (classes.testFlag(PasswordGenerator::SpecialCharacters)) {
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

    m_generator->setLength(m_ui->spinBoxLength->value());
    m_generator->setCharClasses(classes);
    m_generator->setFlags(flags);

    regeneratePassword();
}
