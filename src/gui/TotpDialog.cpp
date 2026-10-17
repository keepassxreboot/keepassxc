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

#include <QAccessible>
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
    updateSeconds();

    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    m_totpUpdateTimer.start(m_step * 10);
    updateTotp();

    new QShortcut(QKeySequence(QKeySequence::Copy), this, SLOT(copyToClipboard()));

    auto* closeButton = m_ui->buttonBox->button(QDialogButtonBox::Cancel);
    auto* copyButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    copyButton->setText(tr("Copy"));
    setTabOrder(m_ui->totpLabel, closeButton);
    setTabOrder(closeButton, copyButton);
    m_ui->totpLabel->setFocusPolicy(Qt::StrongFocus);
    m_ui->totpLabel->setFocus(Qt::OtherFocusReason);

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
    const auto remaining = m_step - (epoch % m_step);
    m_ui->timerLabel->setText(tr("Expires in <b>%n</b> second(s)", "", remaining));

    if (remaining <= 10 && remaining >= 1) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QAccessibleAnnouncementEvent announcementEvent(
            m_ui->timerLabel, tr("TOTP code expires in %n second(s)", "", remaining));
        QAccessible::updateAccessibility(&announcementEvent);
#endif
    }
}

void TotpDialog::updateTotp()
{
    bool isValid = false;
    QString totpCode = m_entry->totp(&isValid);
    if (isValid) {
        totpCode.insert(totpCode.size() / 2, " ");
    }

    auto* copyButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    if (!isValid && copyButton->hasFocus()) {
        m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setFocus(Qt::OtherFocusReason);
    }
    copyButton->setEnabled(isValid);
    m_ui->progressBar->setVisible(isValid);
    m_ui->timerLabel->setVisible(isValid);
    m_ui->totpLabel->setText(totpCode);

    // Keep the code available when the focused label is read, but do not
    // announce the secret automatically through assistive technology.
}

void TotpDialog::resetCounter()
{
    uint epoch = Clock::currentSecondsSinceEpoch();
    m_counter = static_cast<int>(static_cast<double>(epoch % m_step) / m_step * 100);
}
