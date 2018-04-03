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

#include "TotpDialog.h"
#include "ui_TotpDialog.h"

#include "core/Config.h"
#include "gui/Clipboard.h"

TotpDialog::TotpDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpDialog())
    , m_totpUpdateTimer(new QTimer(entry))
    , m_entry(entry)
{
    m_ui->setupUi(this);

    m_step = m_entry->totpStep();
    uCounter = resetCounter();
    updateProgressBar();

    connect(m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    connect(m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    m_totpUpdateTimer->start(m_step * 10);
    updateTotp();

    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
}

void TotpDialog::copyToClipboard()
{
    clipboard()->setText(m_entry->totp());
    if (config()->get("MinimizeOnCopy").toBool()) {
        qobject_cast<DatabaseWidget*>(parent())->window()->showMinimized();
    }
}

void TotpDialog::updateProgressBar()
{
    if (uCounter < 100) {
        m_ui->progressBar->setValue(static_cast<int>(100 - uCounter));
        m_ui->progressBar->update();
        uCounter++;
    } else {
        updateTotp();
        uCounter = resetCounter();
    }
}

void TotpDialog::updateSeconds()
{
    uint epoch = QDateTime::currentDateTime().toTime_t() - 1;
    m_ui->timerLabel->setText(tr("Expires in <b>%n</b> second(s)", "", m_step - (epoch % m_step)));
}

void TotpDialog::updateTotp()
{
    QString totpCode = m_entry->totp();
    QString firstHalf = totpCode.left(totpCode.size() / 2);
    QString secondHalf = totpCode.mid(totpCode.size() / 2);
    m_ui->totpLabel->setText(firstHalf + " " + secondHalf);
}

double TotpDialog::resetCounter()
{
    uint epoch = QDateTime::currentDateTime().toTime_t();
    double counter = qRound(static_cast<double>(epoch % m_step) / m_step * 100);
    return counter;
}

TotpDialog::~TotpDialog()
{
    if (m_totpUpdateTimer) {
        delete m_totpUpdateTimer;
    }
}
