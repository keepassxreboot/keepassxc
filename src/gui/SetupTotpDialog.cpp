/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
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

#include "SetupTotpDialog.h"
#include "ui_SetupTotpDialog.h"
#include "totp/totp.h"


SetupTotpDialog::SetupTotpDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::SetupTotpDialog())
{
    m_entry = entry;
    m_parent = parent;

    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    this->setFixedSize(this->sizeHint());

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(setupTotp()));
    connect(m_ui->customSettingsCheckBox, SIGNAL(toggled(bool)), SLOT(toggleCustom(bool)));
}


void SetupTotpDialog::setupTotp()
{
    quint8 digits;

    if (m_ui->radio8Digits->isChecked()) {
        digits = 8;
    } else {
        digits = 6;
    }

    quint8 step = m_ui->stepSpinBox->value();
    QString seed = QTotp::parseOtpString(m_ui->seedEdit->text(), digits, step);
    m_entry->setTotp(seed, step, digits);
    emit m_parent->entrySelectionChanged();
    close();
}

void SetupTotpDialog::toggleCustom(bool status)
{
    m_ui->digitsLabel->setEnabled(status);
    m_ui->radio6Digits->setEnabled(status);
    m_ui->radio8Digits->setEnabled(status);

    m_ui->stepLabel->setEnabled(status);
    m_ui->stepSpinBox->setEnabled(status);
}


void SetupTotpDialog::setSeed(QString value)
{
    m_ui->seedEdit->setText(value);
}

void SetupTotpDialog::setStep(quint8 step)
{
    m_ui->stepSpinBox->setValue(step);

    if (step != QTotp::defaultStep) {
        m_ui->customSettingsCheckBox->setChecked(true);
    }
}

void SetupTotpDialog::setDigits(quint8 digits)
{
    if (digits == 8) {
        m_ui->radio8Digits->setChecked(true);
        m_ui->radio6Digits->setChecked(false);
    } else {
        m_ui->radio6Digits->setChecked(true);
        m_ui->radio8Digits->setChecked(false);
    }

    if (digits != QTotp::defaultDigits) {
        m_ui->customSettingsCheckBox->setChecked(true);
    }
}


SetupTotpDialog::~SetupTotpDialog()
{
}
