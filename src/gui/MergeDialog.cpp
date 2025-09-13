/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "MergeDialog.h"
#include "ui_MergeDialog.h"

#include "core/Database.h"

#include <QPushButton>
#include <QShortcut>

MergeDialog::MergeDialog(QSharedPointer<Database> source, QSharedPointer<Database> target, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::MergeDialog())
    , m_headerContextMenu(new QMenu())
    , m_sourceDatabase(std::move(source))
    , m_targetDatabase(std::move(target))
{
    setAttribute(Qt::WA_DeleteOnClose);
    // block input to other windows since other interactions can lead to unexpected merge results
    setWindowModality(Qt::WindowModality::ApplicationModal);

    m_ui->setupUi(this);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Merge"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &MergeDialog::cancelMerge);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &MergeDialog::performMerge);

    setupChangeTable();
    updateChangeTable();
}

MergeDialog::MergeDialog(const Merger::ChangeList& changes, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::MergeDialog())
    , m_headerContextMenu(new QMenu())
    , m_changes(changes)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    m_ui->buttonBox->button(QDialogButtonBox::Abort)->hide();

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &MergeDialog::close);

    setupChangeTable();
}

MergeDialog::~MergeDialog() = default;

QVector<MergeDialog::MergeDialogColumns> MergeDialog::columns()
{
    return {MergeDialogColumns::Group,
            MergeDialogColumns::Title,
            MergeDialogColumns::Uuid,
            MergeDialogColumns::Type,
            MergeDialogColumns::Details};
}

int MergeDialog::columnIndex(MergeDialogColumns column)
{
    return columns().indexOf(column);
}

QString MergeDialog::columnName(MergeDialogColumns column)
{
    switch (column) {
    case MergeDialogColumns::Group:
        return tr("Group");
    case MergeDialogColumns::Title:
        return tr("Title");
    case MergeDialogColumns::Uuid:
        return tr("UUID");
    case MergeDialogColumns::Type:
        return tr("Change");
    case MergeDialogColumns::Details:
        return tr("Details");
    }
    return {};
}

QString MergeDialog::cellValue(const Merger::Change& change, MergeDialogColumns column)
{
    switch (column) {
    case MergeDialogColumns::Group:
        return change.group();
    case MergeDialogColumns::Title:
        return change.title();
    case MergeDialogColumns::Uuid:
        if (!change.uuid().isNull()) {
            return change.uuid().toString();
        }
        break;
    case MergeDialogColumns::Type:
        return change.typeString();
    case MergeDialogColumns::Details:
        return change.details();
    }
    return {};
}

bool MergeDialog::isColumnHiddenByDefault(MergeDialogColumns column)
{
    return column == MergeDialogColumns::Uuid;
}

void MergeDialog::setupChangeTable()
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_ui->changeTable);

    m_ui->changeTable->verticalHeader()->setVisible(false);
    m_ui->changeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    m_ui->changeTable->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    m_ui->changeTable->setShowGrid(false);
    m_ui->changeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->changeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->changeTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Create the header context menu
    for (auto column : columns()) {
        auto* action = new QAction(columnName(column), this);
        action->setCheckable(true);
        action->setChecked(!isColumnHiddenByDefault(column));
        connect(action, &QAction::toggled, [this, column](bool checked) {
            m_ui->changeTable->setColumnHidden(columnIndex(column), !checked);
            m_ui->changeTable->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
        });
        m_ui->changeTable->horizontalHeader()->addAction(action);
    }
}

void MergeDialog::updateChangeTable()
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_ui->changeTable);
    Q_ASSERT(m_sourceDatabase.get());
    Q_ASSERT(m_targetDatabase.get());

    m_changes = Merger(m_sourceDatabase.data(), m_targetDatabase.get()).merge(true);

    m_ui->changeTable->clear();

    auto allColumns = columns();
    m_ui->changeTable->setColumnCount(allColumns.size());
    m_ui->changeTable->setRowCount(m_changes.size());
    for (auto column : allColumns) {
        auto name = columnName(column);
        auto index = columnIndex(column);

        m_ui->changeTable->setHorizontalHeaderItem(index, new QTableWidgetItem(name));
        m_ui->changeTable->setColumnHidden(index, isColumnHiddenByDefault(column));
    }
    for (int row = 0; row < m_changes.size(); ++row) {
        const auto& change = m_changes[row];
        for (auto column : allColumns) {
            m_ui->changeTable->setItem(row, columnIndex(column), new QTableWidgetItem(cellValue(change, column)));
        }
    }

    m_ui->changeTable->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
}

void MergeDialog::performMerge()
{
    auto changes = Merger(m_sourceDatabase.data(), m_targetDatabase.data()).merge();
    if (changes != m_changes) {
        qWarning("Merge results differed from the expected changes. Expected: %d, Actual: %d",
                 m_changes.size(),
                 changes.size());
    }

    emit databaseMerged(!changes.isEmpty());
    done(QDialog::Accepted);
}

void MergeDialog::cancelMerge()
{
    done(QDialog::Rejected);
}
