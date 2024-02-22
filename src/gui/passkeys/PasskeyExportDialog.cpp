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

#include "PasskeyExportDialog.h"
#include "ui_PasskeyExportDialog.h"

#include "core/Entry.h"
#include "gui/FileDialog.h"

PasskeyExportDialog::PasskeyExportDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::PasskeyExportDialog())
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_ui->setupUi(this);

    connect(m_ui->exportButton, SIGNAL(clicked()), SLOT(accept()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), SLOT(reject()));
    connect(m_ui->itemsTable->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this,
            SLOT(selectionChanged()));
}

PasskeyExportDialog::~PasskeyExportDialog()
{
}

void PasskeyExportDialog::setEntries(const QList<Entry*>& items)
{
    m_ui->itemsTable->setRowCount(items.count());
    m_ui->itemsTable->setColumnCount(1);

    int row = 0;
    for (const auto& entry : items) {
        auto item = new QTableWidgetItem();
        item->setText(entry->title() + " - " + entry->username());
        item->setData(Qt::UserRole, row);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        m_ui->itemsTable->setItem(row, 0, item);

        ++row;
    }
    m_ui->itemsTable->resizeColumnsToContents();
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->itemsTable->selectAll();
    m_ui->exportButton->setFocus();
}

QList<QTableWidgetItem*> PasskeyExportDialog::getSelectedItems() const
{
    QList<QTableWidgetItem*> selected;
    for (int i = 0; i < m_ui->itemsTable->rowCount(); ++i) {
        auto item = m_ui->itemsTable->item(i, 0);
        if (item->isSelected()) {
            selected.append(item);
        }
    }
    return selected;
}

void PasskeyExportDialog::selectionChanged()
{
    auto indexes = m_ui->itemsTable->selectionModel()->selectedIndexes();
    m_ui->exportButton->setEnabled(!indexes.isEmpty());

    if (indexes.isEmpty()) {
        m_ui->exportButton->clearFocus();
        m_ui->cancelButton->setFocus();
    }
}

QString PasskeyExportDialog::selectExportFolder()
{
    return fileDialog()->getExistingDirectory(this, tr("Export to folder"), FileDialog::getLastDir("passkey"));
}
