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

#include "MergeDialog.h"
#include "ui_MergeDialog.h"

#include "core/Database.h"

#include <QPushButton>
#include <QShortcut>
#include <qdebug.h>

MergeDialog::MergeDialog(QSharedPointer<Database> source, QSharedPointer<Database> target, QWidget* parent)
    : QDialog(parent)
    , m_merger(source.data(), target.data())
    , m_sourceDatabase{std::move(source)}
    , m_targetDatabase{std::move(target)}
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui.setupUi(this);

    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Merge"));
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setFocus();

    connect(m_ui.buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui.buttonBox, SIGNAL(accepted()), SLOT(performMerge()));

    setupChangeTable();

    // block input to other windows since other interactions can lead to unexpected merge results
    setWindowModality(Qt::WindowModality::ApplicationModal);
}

void MergeDialog::setupChangeTable()
{
    auto changeList = m_merger.changes();

    auto* table = m_ui.changeTable;
    auto columns = QVector<QPair<QString, std::function<QString(const Merger::Change&)>>>{
        qMakePair(tr("Group"), [](const auto& change) { return change.group(); }),
        qMakePair(tr("Title"), [](const auto& change) { return change.title(); }),
        qMakePair(tr("UUID"),
                  [](const auto& change) {
                      if (!change.uuid().isNull()) {
                          return change.uuid().toString();
                      }
                      return QString();
                  }),
        qMakePair(tr("Type of change"), [](const auto& change) { return change.typeString(); }),
        qMakePair(tr("Details"), [](const auto& change) { return change.details(); })};
    table->setColumnCount(columns.size());
    table->setRowCount(changeList.size());
    for (int column = 0; column < columns.size(); ++column) {
        const auto& columnName = columns[column].first;
        table->setHorizontalHeaderItem(column, new QTableWidgetItem(columnName));
    }
    for (int row = 0; row < changeList.size(); ++row) {
        const auto& change = changeList[row];
        for (int column = 0; column < columns.size(); ++column) {
            auto changeMember = columns[column].second;
            table->setItem(row, column, new QTableWidgetItem(changeMember(change)));
        }
    }

    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);

    table->setShowGrid(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MergeDialog::performMerge()
{
    auto changelist = m_merger.merge(false);
    emit databaseMerged(!changelist.isEmpty());
    done(QDialog::Accepted);
}
