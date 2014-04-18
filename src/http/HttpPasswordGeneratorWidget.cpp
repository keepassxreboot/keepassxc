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

#include "HttpPasswordGeneratorWidget.h"
#include "ui_HttpPasswordGeneratorWidget.h"

#include <QLineEdit>

#include "core/Config.h"
#include "core/PasswordGenerator.h"
#include "core/FilePath.h"

HttpPasswordGeneratorWidget::HttpPasswordGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingSpinBox(false)
    , m_generator(new PasswordGenerator())
    , m_ui(new Ui::HttpPasswordGeneratorWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(saveSettings()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(sliderMoved()));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(spinBoxChanged()));

    connect(m_ui->optionButtons, SIGNAL(buttonClicked(int)), SLOT(updateGenerator()));
    m_ui->buttonApply->setEnabled(true);

    loadSettings();
    reset();
}

HttpPasswordGeneratorWidget::~HttpPasswordGeneratorWidget()
{
}

void HttpPasswordGeneratorWidget::loadSettings()
{
    m_ui->checkBoxLower->setChecked(config()->get("Http/generator/LowerCase", true).toBool());
    m_ui->checkBoxUpper->setChecked(config()->get("Http/generator/UpperCase", true).toBool());
    m_ui->checkBoxNumbers->setChecked(config()->get("Http/generator/Numbers", true).toBool());
    m_ui->checkBoxSpecialChars->setChecked(config()->get("Http/generator/SpecialChars", false).toBool());

    m_ui->checkBoxExcludeAlike->setChecked(config()->get("Http/generator/ExcludeAlike", true).toBool());
    m_ui->checkBoxEnsureEvery->setChecked(config()->get("Http/generator/EnsureEvery", true).toBool());

    m_ui->spinBoxLength->setValue(config()->get("Http/generator/Length", 16).toInt());
}

void HttpPasswordGeneratorWidget::saveSettings()
{
    config()->set("Http/generator/LowerCase", m_ui->checkBoxLower->isChecked());
    config()->set("Http/generator/UpperCase", m_ui->checkBoxUpper->isChecked());
    config()->set("Http/generator/Numbers", m_ui->checkBoxNumbers->isChecked());
    config()->set("Http/generator/SpecialChars", m_ui->checkBoxSpecialChars->isChecked());

    config()->set("Http/generator/ExcludeAlike", m_ui->checkBoxExcludeAlike->isChecked());
    config()->set("Http/generator/EnsureEvery", m_ui->checkBoxEnsureEvery->isChecked());

    config()->set("Http/generator/Length", m_ui->spinBoxLength->value());
}

void HttpPasswordGeneratorWidget::reset()
{
    updateGenerator();
}

void HttpPasswordGeneratorWidget::sliderMoved()
{
    if (m_updatingSpinBox) {
        return;
    }

    m_ui->spinBoxLength->setValue(m_ui->sliderLength->value());

    updateGenerator();
}

void HttpPasswordGeneratorWidget::spinBoxChanged()
{
    // Interlock so that we don't update twice - this causes issues as the spinbox can go higher than slider
    m_updatingSpinBox = true;

    m_ui->sliderLength->setValue(m_ui->spinBoxLength->value());

    m_updatingSpinBox = false;

    updateGenerator();
}

PasswordGenerator::CharClasses HttpPasswordGeneratorWidget::charClasses()
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

PasswordGenerator::GeneratorFlags HttpPasswordGeneratorWidget::generatorFlags()
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

void HttpPasswordGeneratorWidget::updateGenerator()
{
    m_generator->setLength(m_ui->spinBoxLength->value());
    m_generator->setCharClasses(charClasses());
    m_generator->setFlags(generatorFlags());
}
