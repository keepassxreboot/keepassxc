/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BrowserPasskeysConfirmationDialog.h"
#include "ui_BrowserPasskeysConfirmationDialog.h"

#include "core/Entry.h"
#include <QCloseEvent>
#include <QUrl>

#define STEP 1000

BrowserPasskeysConfirmationDialog::BrowserPasskeysConfirmationDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::BrowserPasskeysConfirmationDialog())
    , m_passkeyUpdated(false)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);
    m_ui->updateButton->setVisible(false);
    m_ui->verticalLayout->setAlignment(Qt::AlignTop);

    connect(m_ui->credentialsTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(accept()));
    connect(m_ui->confirmButton, SIGNAL(clicked()), SLOT(accept()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), SLOT(reject()));
    connect(m_ui->updateButton, SIGNAL(clicked()), SLOT(updatePasskey()));

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateSeconds()));
}

BrowserPasskeysConfirmationDialog::~BrowserPasskeysConfirmationDialog()
{
}

void BrowserPasskeysConfirmationDialog::registerCredential(const QString& username,
                                                           const QString& relyingParty,
                                                           const QList<Entry*>& existingEntries,
                                                           int timeout)
{
    m_ui->firstLabel->setText(tr("Do you want to register Passkey for:"));
    m_ui->relyingPartyLabel->setText(tr("Relying Party: %1").arg(relyingParty));
    m_ui->usernameLabel->setText(tr("Username: %1").arg(username));
    m_ui->secondLabel->setText("");

    if (!existingEntries.isEmpty()) {
        m_ui->firstLabel->setText(tr("Existing Passkey found.\nDo you want to register a new Passkey for:"));
        m_ui->secondLabel->setText(tr("Select the existing Passkey and press Update to replace it."));

        m_ui->updateButton->setVisible(true);
        m_ui->confirmButton->setText(tr("Register new"));
        updateEntriesToTable(existingEntries);
    } else {
        m_ui->verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        m_ui->confirmButton->setText(tr("Register"));
        m_ui->credentialsTable->setVisible(false);
    }

    startCounter(timeout);
}

void BrowserPasskeysConfirmationDialog::authenticateCredential(const QList<Entry*>& entries,
                                                               const QString& relyingParty,
                                                               int timeout)
{
    m_ui->firstLabel->setText(tr("Authenticate Passkey credentials for:"));
    m_ui->relyingPartyLabel->setText(tr("Relying Party: %1").arg(relyingParty));
    m_ui->usernameLabel->setVisible(false);
    m_ui->secondLabel->setText("");
    updateEntriesToTable(entries);
    startCounter(timeout);
}

Entry* BrowserPasskeysConfirmationDialog::getSelectedEntry() const
{
    auto selectedItem = m_ui->credentialsTable->currentItem();
    return selectedItem ? m_entries[selectedItem->row()] : nullptr;
}

bool BrowserPasskeysConfirmationDialog::isPasskeyUpdated() const
{
    return m_passkeyUpdated;
}

void BrowserPasskeysConfirmationDialog::updatePasskey()
{
    m_passkeyUpdated = true;
    emit accept();
}

void BrowserPasskeysConfirmationDialog::updateProgressBar()
{
    if (m_counter < m_ui->progressBar->maximum()) {
        m_ui->progressBar->setValue(m_ui->progressBar->maximum() - m_counter);
        m_ui->progressBar->update();
    } else {
        emit reject();
    }
}

void BrowserPasskeysConfirmationDialog::updateSeconds()
{
    ++m_counter;
    updateTimeoutLabel();
}

void BrowserPasskeysConfirmationDialog::startCounter(int timeout)
{
    m_counter = 0;
    m_ui->progressBar->setMaximum(timeout / STEP);
    updateProgressBar();
    updateTimeoutLabel();
    m_timer.start(STEP);
}

void BrowserPasskeysConfirmationDialog::updateTimeoutLabel()
{
    m_ui->timeoutLabel->setText(tr("Timeout in <b>%n</b> seconds...", "", m_ui->progressBar->maximum() - m_counter));
}

void BrowserPasskeysConfirmationDialog::updateEntriesToTable(const QList<Entry*>& entries)
{
    m_entries = entries;
    m_ui->credentialsTable->setRowCount(entries.count());
    m_ui->credentialsTable->setColumnCount(1);

    int row = 0;
    for (const auto& entry : entries) {
        auto item = new QTableWidgetItem();
        item->setText(entry->title() + " - " + entry->username());
        m_ui->credentialsTable->setItem(row, 0, item);

        if (row == 0) {
            item->setSelected(true);
        }

        ++row;
    }

    m_ui->credentialsTable->resizeColumnsToContents();
    m_ui->credentialsTable->horizontalHeader()->setStretchLastSection(true);
}