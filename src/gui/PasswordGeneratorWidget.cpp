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

PasswordGeneratorWidget::PasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::PasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->editNewPassword, SIGNAL(textChanged(QString)), SLOT(updateApplyEnabled(QString)));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_ui->buttonGenerate, SIGNAL(clicked()), SLOT(generatePassword()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(emitNewPassword()));

    reset();
}

PasswordGeneratorWidget::~PasswordGeneratorWidget()
{
}

void PasswordGeneratorWidget::reset()
{
    m_ui->checkBoxLower->setChecked(true);
    m_ui->checkBoxUpper->setChecked(true);
    m_ui->checkBoxNumbers->setChecked(true);
    m_ui->checkBoxSpecialChars->setChecked(false);

    m_ui->checkBoxExcludeAlike->setChecked(true);
    m_ui->checkBoxEnsureEvery->setChecked(true);

    m_ui->spinBoxLength->setValue(16);

    m_ui->editNewPassword->setText("");
    m_ui->togglePasswordButton->setChecked(true);
}

void PasswordGeneratorWidget::updateApplyEnabled(const QString& password)
{
    m_ui->buttonApply->setEnabled(!password.isEmpty());
}

void PasswordGeneratorWidget::togglePassword(bool checked)
{
    m_ui->editNewPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

void PasswordGeneratorWidget::generatePassword()
{
    int length = m_ui->spinBoxLength->value();
    PasswordGenerator::CharClasses classes = charClasses();
    PasswordGenerator::GeneratorFlags flags = generatorFlags();

    if (!passwordGenerator()->isValidCombination(length, classes, flags)) {
        // TODO: display error
        return;
    }

    QString password = passwordGenerator()->generatePassword(length, classes, flags);
    m_ui->editNewPassword->setText(password);
}

void PasswordGeneratorWidget::emitNewPassword()
{
    Q_EMIT newPassword(m_ui->editNewPassword->text());
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
