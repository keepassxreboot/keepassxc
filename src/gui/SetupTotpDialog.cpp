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
#include "totp/totp.h"
#include "ui_SetupTotpDialog.h"

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
    connect(m_ui->radioDefault, SIGNAL(toggled(bool)), SLOT(toggleDefault(bool)));
    connect(m_ui->radioSteam, SIGNAL(toggled(bool)), SLOT(toggleSteam(bool)));
    connect(m_ui->radioCustom, SIGNAL(toggled(bool)), SLOT(toggleCustom(bool)));
}

void SetupTotpDialog::setupTotp()
{
    quint8 digits;

    if (m_ui->radioSteam->isChecked()) {
        digits = Totp::ENCODER_STEAM;
    } else if (m_ui->radio8Digits->isChecked()) {
        digits = 8;
    } else {
        digits = 6;
    }

    quint8 step = m_ui->stepSpinBox->value();
    QString seed = Totp::parseOtpString(m_ui->seedEdit->text(), digits, step);
    m_entry->setTotp(seed, step, digits);
    emit m_parent->entrySelectionChanged();
    close();
}

void SetupTotpDialog::toggleDefault(bool status)
{
    if (status) {
        setStep(Totp::defaultStep);
        setDigits(Totp::defaultDigits);
    }
}

void SetupTotpDialog::toggleSteam(bool status)
{
    if (status) {
        setStep(Totp::defaultStep);
        setDigits(Totp::ENCODER_STEAM);
    }
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

void SetupTotpDialog::setSettings(quint8 digits)
{
    quint8 step = m_ui->stepSpinBox->value();

    bool isDefault = ((step == Totp::defaultStep) && (digits == Totp::defaultDigits));
    bool isSteam = (digits == Totp::ENCODER_STEAM);

    if (isSteam) {
        m_ui->radioSteam->setChecked(true);
    } else if (isDefault) {
        m_ui->radioDefault->setChecked(true);
    } else {
        m_ui->radioCustom->setChecked(true);
    }
}

void SetupTotpDialog::setStep(quint8 step)
{
    m_ui->stepSpinBox->setValue(step);
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
}

SetupTotpDialog::~SetupTotpDialog()
{
}
