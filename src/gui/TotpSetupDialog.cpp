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

#include "TotpSetupDialog.h"
#include "ui_TotpSetupDialog.h"

#include "totp/totp.h"

TotpSetupDialog::TotpSetupDialog(QWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpSetupDialog())
    , m_entry(entry)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(sizeHint());

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(m_ui->radioCustom, SIGNAL(toggled(bool)), SLOT(toggleCustom(bool)));

    init();
}

TotpSetupDialog::~TotpSetupDialog()
{
}

void TotpSetupDialog::saveSettings()
{
    QString encShortName;
    uint digits = Totp::DEFAULT_DIGITS;
    uint step = Totp::DEFAULT_STEP;

    if (m_ui->radioSteam->isChecked()) {
        digits = Totp::STEAM_DIGITS;
        encShortName = Totp::STEAM_SHORTNAME;
    } else if (m_ui->radioCustom->isChecked()) {
        step = m_ui->stepSpinBox->value();
        if (m_ui->radio8Digits->isChecked()) {
            digits = 8;
        } else if (m_ui->radio7Digits->isChecked()) {
            digits = 7;
        }
    }

    auto settings = Totp::createSettings(m_ui->seedEdit->text(), digits, step, encShortName, m_entry->totpSettings());
    m_entry->setTotp(settings);
    emit totpUpdated();
    close();
}

void TotpSetupDialog::toggleCustom(bool status)
{
    m_ui->customGroup->setEnabled(status);
}

void TotpSetupDialog::init()
{
    auto settings = m_entry->totpSettings();
    if (!settings.isNull()) {
        m_ui->seedEdit->setText(settings->key);
        m_ui->stepSpinBox->setValue(settings->step);

        if (settings->encoder.shortName == Totp::STEAM_SHORTNAME) {
            m_ui->radioSteam->setChecked(true);
        } else if (settings->custom) {
            m_ui->radioCustom->setChecked(true);
            if (settings->digits == 8) {
                m_ui->radio8Digits->setChecked(true);
            } else if (settings->digits == 7) {
                m_ui->radio7Digits->setChecked(true);
            } else {
                m_ui->radio6Digits->setChecked(true);
            }
        }
    }
}
