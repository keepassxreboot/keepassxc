/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "AccessControlDialog.h"
#include "ui_AccessControlDialog.h"

#include "core/Entry.h"

#include <QPushButton>
#include <QTableWidgetItem>
#include <QWindow>

AccessControlDialog::AccessControlDialog(QWindow* parent, const QList<Entry*>& items, const QString& name, uint pid)
    : m_ui(new Ui::AccessControlDialog())
{
    if (parent) {
        // Force the creation of the QWindow, without this windowHandle() will return nullptr
        winId();
        auto window = windowHandle();
        Q_ASSERT(window);
        window->setTransientParent(parent);
    }

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);

    connect(m_ui->cancelButton, &QPushButton::clicked, this, &AccessControlDialog::reject);
    connect(m_ui->allowButton, &QPushButton::clicked, this, &AccessControlDialog::accept);
    connect(m_ui->allowAllButton, &QPushButton::clicked, this, [this]() { this->done(AccessControlDialog::AllowAll); });

    m_ui->appLabel->setText(m_ui->appLabel->text().arg(name).arg(pid));

    m_ui->itemsTable->setRowCount(items.count());
    m_ui->itemsTable->setColumnCount(2);

    for (const auto& entry : asConst(items)) {
        auto row = m_ui->itemsTable->rowCount();

        auto item = new QTableWidgetItem();
        item->setText(entry->title() + " - " + entry->username());
        // save the original row index which is stable regardless of removals
        item->setData(Qt::UserRole, row);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        auto disableButton = new QPushButton(tr("Disable for this site"));
        disableButton->setAutoDefault(false);
        connect(disableButton, &QAbstractButton::pressed, this, [this, item]() { disableButtonPressed(item); });

        m_ui->itemsTable->setItem(row, 0, item);
        m_ui->itemsTable->setCellWidget(row, 1, disableButton);
    }

    m_ui->itemsTable->resizeColumnsToContents();
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_ui->allowButton->setFocus();
}

void AccessControlDialog::disableButtonPressed(QTableWidgetItem* item)
{
    emit disableAccess(qvariant_cast<Entry*>(item->data(Qt::UserRole)));
    m_ui->itemsTable->removeRow(item->row());
    if (m_ui->itemsTable->rowCount() == 0) {
        reject();
    }
}

AccessControlDialog::~AccessControlDialog() = default;

QList<int> AccessControlDialog::getEntryIndices(bool selected) const
{
    QList<int> entries;
    for (int i = 0; i < m_ui->itemsTable->rowCount(); ++i) {
        auto item = m_ui->itemsTable->item(i, 0);
        if ((item->checkState() == Qt::Checked) == selected) {
            entries << item->data(Qt::UserRole).toInt();
        }
    }
    return entries;
}
