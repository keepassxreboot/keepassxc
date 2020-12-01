/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include <QWindow>
#include <QAbstractTableModel>

#include <utility>

AccessControlDialog::AccessControlDialog(QWindow* parent, const QList<Entry*>& items, const QString& name, uint pid)
    : m_ui(new Ui::AccessControlDialog())
    , m_model(new EntryModel(items))
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
    // click anywhere in the row to check/uncheck the item
    connect(m_ui->itemsTable, &QTableView::clicked, m_model.data(), &EntryModel::toggleCheckState);

    m_ui->appLabel->setText(m_ui->appLabel->text().arg(name).arg(pid));
    m_ui->itemsTable->setModel(m_model.data());

    // resize table columns
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->itemsTable->resizeColumnsToContents();

    m_ui->allowButton->setFocus();
}

AccessControlDialog::~AccessControlDialog() = default;

QList<int> AccessControlDialog::getEntryIndices() const
{
    return m_model->selectedIndices();
}

AccessControlDialog::EntryModel::EntryModel(QList<Entry*> entries, QObject* parent)
    : QAbstractTableModel(parent)
      , m_entries(std::move(entries))
{
    for (int i = 0; i != m_entries.count(); ++i) {
        m_selected.insert(i);
    }
}

int AccessControlDialog::EntryModel::rowCount(const QModelIndex& parent) const
{
    return isValid(parent) ? 0 : m_entries.count();
}

int AccessControlDialog::EntryModel::columnCount(const QModelIndex& parent) const
{
    return isValid(parent) ? 0 : 2;
}

bool AccessControlDialog::EntryModel::isValid(const QModelIndex& index) const {
    return index.isValid() && index.row() < rowCount({}) && index.column() < columnCount({});
}

QList<int> AccessControlDialog::EntryModel::selectedIndices() const
{
    return m_selected.toList();
}

void AccessControlDialog::EntryModel::toggleCheckState(const QModelIndex& index)
{
    if (!isValid(index)) {
        return;
    }
    auto it = m_selected.find(index.row());
    if (it == m_selected.end()) {
        m_selected.insert(index.row());
    } else {
        m_selected.erase(it);
    }
    auto rowIdx = index.sibling(index.row(), 0);
    emit dataChanged(rowIdx, rowIdx, {Qt::CheckStateRole});
}

QVariant AccessControlDialog::EntryModel::data(const QModelIndex& index, int role) const
{
    if (!isValid(index)) {
        return {};
    }
    auto entry = m_entries.at(index.row());

    switch (index.column()) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return entry->title();
        case Qt::DecorationRole:
            return entry->icon();
        case Qt::CheckStateRole:
            return m_selected.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
        default:
            return {};
        }
    case 1:
        switch (role) {
        case Qt::DisplayRole:
            return entry->username();
        default:
            return {};
        }
    default:
        return {};
    }
}
