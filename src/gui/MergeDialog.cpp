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

    m_ui->setupUi(this);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Merge"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &MergeDialog::abortMerge);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &MergeDialog::performMerge);

    setupChangeTable();
    setupHeaderContextMenu();

    refreshMergeChanges();

    // block input to other windows since other interactions can lead to unexpected merge results
    setWindowModality(Qt::WindowModality::ApplicationModal);
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
    setupHeaderContextMenu();
}

MergeDialog::~MergeDialog() = default;

QSharedPointer<Database> MergeDialog::createTemporaryTargetDatabase()
{
    auto tmpDatabase = m_targetDatabase->clone();
    // make sure temporary merge will not overwrite actual database
    tmpDatabase->setFilePath("");
    tmpDatabase->markAsTemporaryDatabase();
    return tmpDatabase;
}

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
    return column == MergeDialogColumns::Uuid || column == MergeDialogColumns::Details;
}

void MergeDialog::calculateChanges()
{
    auto tmpDatabase = createTemporaryTargetDatabase();
    m_changes = Merger(m_sourceDatabase.data(), tmpDatabase.get()).merge();
}

void MergeDialog::setupChangeTable()
{
    assert(m_ui);
    auto* table = m_ui->changeTable;
    assert(table);

    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    table->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    table->setShowGrid(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MergeDialog::setupHeaderContextMenu()
{
    for (auto column : columns()) {
        auto* action = new QAction(columnName(column), this);
        action->setCheckable(true);
        action->setChecked(!isColumnHiddenByDefault(column));
        connect(action, &QAction::toggled, [this, column](bool checked) {
            auto* table = m_ui->changeTable;
            table->setColumnHidden(columnIndex(column), !checked);
            table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
        });
        m_ui->changeTable->horizontalHeader()->addAction(action);
    }
}

void MergeDialog::updateChangeTable()
{
    assert(m_ui);
    auto* table = m_ui->changeTable;
    assert(table);

    table->clear();

    auto allColumns = columns();
    table->setColumnCount(allColumns.size());
    table->setRowCount(m_changes.size());
    for (auto column : allColumns) {
        auto name = columnName(column);
        auto index = columnIndex(column);

        table->setHorizontalHeaderItem(index, new QTableWidgetItem(name));
        table->setColumnHidden(index, isColumnHiddenByDefault(column));
    }
    for (int row = 0; row < m_changes.size(); ++row) {
        const auto& change = m_changes[row];
        for (auto column : allColumns) {
            auto value = cellValue(change, column);
            table->setItem(row, columnIndex(column), new QTableWidgetItem(value));
        }
    }

    table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
}

void MergeDialog::performMerge()
{
    auto changes = Merger(m_sourceDatabase.data(), m_targetDatabase.data()).merge();
    if (changes != m_changes) {
        emit databaseModifiedMerge(changes, m_changes);
    } else {
        emit databaseMerged(!changes.isEmpty());
    }
    done(QDialog::Accepted);
}

void MergeDialog::abortMerge()
{
    done(QDialog::Rejected);
}

void MergeDialog::refreshMergeChanges()
{
    calculateChanges();
    updateChangeTable();
}
