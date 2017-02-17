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

#include "PassphraseGeneratorWidget.h"
#include "ui_PassphraseGeneratorWidget.h"

#include <QLineEdit>

#include "core/Config.h"
#include "core/PassphraseGenerator.h"
#include "core/FilePath.h"

PassphraseGeneratorWidget::PassphraseGeneratorWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingSpinBox(false)
    , m_generator(new PassphraseGenerator())
    , m_ui(new Ui::PassphraseGeneratorWidget())
{
    m_ui->setupUi(this);

    m_ui->togglePassphraseButton->setIcon(filePath()->onOffIcon("actions", "password-show"));

    connect(m_ui->editNewPassphrase->lineEdit(), SIGNAL(textChanged(QString)), SLOT(updateApplyEnabled(QString)));
    connect(m_ui->togglePassphraseButton, SIGNAL(toggled(bool)), m_ui->editNewPassphrase, SLOT(setEcho(bool)));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(emitNewPassphrase()));
    connect(m_ui->buttonApply, SIGNAL(clicked()), SLOT(saveSettings()));

    connect(m_ui->sliderLength, SIGNAL(valueChanged(int)), SLOT(sliderMoved()));
    connect(m_ui->spinBoxLength, SIGNAL(valueChanged(int)), SLOT(spinBoxChanged()));

    m_ui->editNewPassphrase->setGenerator(m_generator.data());

    loadSettings();
    reset();
}

PassphraseGeneratorWidget::~PassphraseGeneratorWidget()
{
}

void PassphraseGeneratorWidget::loadSettings()
{
    m_ui->spinBoxLength->setValue(config()->get("passphrasegenerator/Length", 40).toInt());
}

void PassphraseGeneratorWidget::saveSettings()
{
    config()->set("passphrasegenerator/Length", m_ui->spinBoxLength->value());
}

void PassphraseGeneratorWidget::reset()
{
    m_ui->editNewPassphrase->lineEdit()->setText("");
    m_ui->togglePassphraseButton->setChecked(config()->get("security/passwordscleartext").toBool());

    updateGenerator();
}

void PassphraseGeneratorWidget::regeneratePassphrase()
{
    if (m_generator->isValid()) {
        QString passphrase = m_generator->generatePassphrase();
        m_ui->editNewPassphrase->setEditText(passphrase);
    }
}

void PassphraseGeneratorWidget::updateApplyEnabled(const QString& password)
{
    m_ui->buttonApply->setEnabled(!password.isEmpty());
}

void PassphraseGeneratorWidget::emitNewPassphrase()
{
    Q_EMIT newPassword(m_ui->editNewPassphrase->lineEdit()->text());
}

void PassphraseGeneratorWidget::sliderMoved()
{
    if (m_updatingSpinBox) {
        return;
    }

    m_ui->spinBoxLength->setValue(m_ui->sliderLength->value());

    updateGenerator();
}

void PassphraseGeneratorWidget::spinBoxChanged()
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

void PassphraseGeneratorWidget::updateGenerator()
{
    int minLength = 1;

    if (m_ui->spinBoxLength->value() < minLength) {
        m_updatingSpinBox = true;
        m_ui->spinBoxLength->setValue(minLength);
        m_ui->sliderLength->setValue(minLength);
        m_updatingSpinBox = false;
    }

    m_ui->spinBoxLength->setMinimum(minLength);
    m_ui->sliderLength->setMinimum(minLength);

    m_generator->setLength(m_ui->spinBoxLength->value());

    regeneratePassphrase();
}
