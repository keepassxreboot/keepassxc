/*
 *  Copyright (C) 2017 Weslly Honorato <weslly@protonmail.com>
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

#include "core/Clock.h"
#include "core/Totp.h"
#include "gui/Clipboard.h"
#include "gui/MainWindow.h"

#include <QPushButton>
#include <QShortcut>

TotpDialog::TotpDialog(QWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpDialog())
    , m_entry(entry)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);

    m_step = m_entry->totpSettings()->step;
    resetCounter();
    updateProgressBar();

    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    m_totpUpdateTimer.start(m_step * 10);
    updateTotp();

    new QShortcut(QKeySequence(QKeySequence::Copy), this, SLOT(copyToClipboard()));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
}

TotpDialog::~TotpDialog() = default;

void TotpDialog::copyToClipboard()
{
    clipboard()->setText(m_entry->totp());
    if (config()->get(Config::HideWindowOnCopy).toBool()) {
        if (config()->get(Config::MinimizeOnCopy).toBool()) {
            getMainWindow()->minimizeOrHide();
        } else if (config()->get(Config::DropToBackgroundOnCopy).toBool()) {
            getMainWindow()->lower();
            window()->lower();
        }
    }
}

void TotpDialog::updateProgressBar()
{
    if (m_counter < 100) {
        m_ui->progressBar->setValue(100 - m_counter);
        m_ui->progressBar->update();
        ++m_counter;
    } else {
        updateTotp();
        resetCounter();
    }
}

void TotpDialog::updateSeconds()
{
    uint epoch = Clock::currentSecondsSinceEpoch() - 1;
    m_ui->timerLabel->setText(tr("Expires in <b>%n</b> second(s)", "", m_step - (epoch % m_step)));
}

void TotpDialog::updateTotp()
{
    QString totpCode = m_entry->totp();
    QString firstHalf = totpCode.left(totpCode.size() / 2);
    QString secondHalf = totpCode.mid(totpCode.size() / 2);
    m_ui->totpLabel->setText(firstHalf + " " + secondHalf);
}

void TotpDialog::resetCounter()
{
    uint epoch = Clock::currentSecondsSinceEpoch();
    m_counter = static_cast<int>(static_cast<double>(epoch % m_step) / m_step * 100);
}
