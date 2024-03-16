/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "GlobalTotpDialog.h"
#include "ui_GlobalTotpDialog.h"

#include "core/Clock.h"
#include "core/Totp.h"
#include "gui/Clipboard.h"
#include "gui/MainWindow.h"

#include <QPushButton>
#include <QShortcut>
#include <QToolButton>

GlobalTotpDialog::GlobalTotpDialog(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::GlobalTotpDialog())
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);

    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateCounter()));
    connect(&m_totpUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTable()));

    new QShortcut(QKeySequence(QKeySequence::Copy), this, SLOT(copyToClipboard()));

    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SIGNAL(closed()));
}

GlobalTotpDialog::~GlobalTotpDialog() = default;

void GlobalTotpDialog::setDatabaseWidget(DatabaseWidget* databaseWidget)
{
    if (!databaseWidget) {
        return;
    }

    m_databaseWidget = databaseWidget;
    m_ui->totpTable->clear();

    m_entries.clear();
    const auto db = m_databaseWidget->database();
    for (const auto& group : db->rootGroup()->groupsRecursive(true)) {
        if (group == db->metadata()->recycleBin()) {
            continue;
        }

        for (const auto& entry : group->entries()) {
            if (entry->hasTotp()) {
                m_entries.push_back(entry);
            }
        }
    }

    m_ui->totpTable->setRowCount(m_entries.size());
    m_ui->totpTable->setColumnCount(5);
    m_ui->totpTable->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Username") << tr("TOTP")
                                                             << tr("Expires in (s)") << tr("Copy TOTP"));
    m_ui->totpTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->totpTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->totpTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_ui->totpTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    uint epoch = Clock::currentSecondsSinceEpoch() - 1;

    auto i = 0;
    for (const auto& entry : m_entries) {
        const auto step = entry->totpSettings()->step;

        m_ui->totpTable->setItem(i, 0, new QTableWidgetItem(entry->title()));
        m_ui->totpTable->setItem(i, 1, new QTableWidgetItem(entry->username()));
        m_ui->totpTable->setItem(i, 2, new QTableWidgetItem(entry->totp()));
        m_ui->totpTable->setItem(i, 3, new QTableWidgetItem(QString::number(step - (epoch % step))));

        auto button = new QToolButton();
        button->setText(tr("Copy"));
        button->setProperty("row", i);

        connect(button, &QToolButton::clicked, this, [this]() {
            auto btn = qobject_cast<QToolButton*>(sender());
            auto totp = m_ui->totpTable->item(btn->property("row").toInt(), 2)->text();

            clipboard()->setText(totp);
        });

        m_ui->totpTable->setCellWidget(i, 4, button);
        ++i;
    }

    m_totpUpdateTimer.start(1000);
}

void GlobalTotpDialog::copyToClipboard()
{
    const auto selectedItems = m_ui->totpTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    const auto selectedRow = selectedItems.first()->row();
    auto totp = m_ui->totpTable->item(selectedRow, 2)->text();
    clipboard()->setText(totp);
}

void GlobalTotpDialog::updateCounter()
{
    if (!isVisible()) {
        m_totpUpdateTimer.stop();
        return;
    }
}

void GlobalTotpDialog::updateTable()
{
    uint epoch = Clock::currentSecondsSinceEpoch() - 1;

    for (auto i = 0; i < m_ui->totpTable->rowCount(); ++i) {
        const auto totpCounter = m_ui->totpTable->item(i, 3)->text().toInt();
        const auto entry = m_entries.at(i);
        const auto step = entry->totpSettings()->step;

        if (totpCounter == 1) {
            m_ui->totpTable->item(i, 2)->setText(entry->totp());
            m_ui->totpTable->item(i, 3)->setText(QString::number(step));
        } else {
            m_ui->totpTable->item(i, 3)->setText(QString::number(step - (epoch % step)));
        }
    }
}
