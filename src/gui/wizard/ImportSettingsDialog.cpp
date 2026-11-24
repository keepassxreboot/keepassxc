/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "ImportSettingsDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
    enum Column
    {
        ApplyColumn = 0,
        SettingColumn,
        CurrentColumn,
        FileColumn,
        ColumnCount
    };
} // namespace

ImportSettingsDialog::ImportSettingsDialog(const QList<DatabaseSettingChange>& changes, QWidget* parent, Mode mode)
    : QDialog(parent)
    , m_changes(changes)
    , m_table(new QTableWidget(changes.size(), ColumnCount, this))
{
    const bool newDatabase = mode == Mode::NewDatabase;
    setWindowTitle(newDatabase ? tr("Settings for the new database") : tr("Database settings in the file"));
    setModal(true);

    // Into a new database the file defines the new database, so everything starts ticked (a complete copy); into an
    // open database the user's own settings stay as they are, so nothing starts ticked.
    auto* intro = new QLabel(
        newDatabase ? tr("The new database takes the folders and entries of the file. The file also holds settings of "
                         "the database it came from, and they differ from the settings of your new database "
                         "(including the name and description you just entered).\n\n"
                         "Everything is ticked, so the file's values are used. Untick any setting you want to keep as "
                         "it is in your new database. Cancel stops the import and no database is created.")
                    : tr("The file also holds settings of the database it came from. Importing into a database "
                         "you already have adds the folders and entries and leaves your database's own "
                         "settings as they are.\n\n"
                         "The settings below are different in the file. Tick the ones you also want to apply; "
                         "they replace your current values. Settings you leave unticked are not changed. "
                         "Cancel stops the whole import and changes nothing."),
        this);
    intro->setWordWrap(true);

    m_table->setHorizontalHeaderLabels({tr("Apply"),
                                        tr("Setting"),
                                        newDatabase ? tr("Your new database now") : tr("Your database now"),
                                        tr("In the file")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setWordWrap(true);

    for (int row = 0; row < changes.size(); ++row) {
        const auto& change = changes.at(row);

        auto* apply = new QTableWidgetItem();
        apply->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        apply->setCheckState(newDatabase ? Qt::Checked : Qt::Unchecked);

        auto* setting = new QTableWidgetItem(change.note.isEmpty() ? change.label : change.label + "\n" + change.note);
        auto* current = new QTableWidgetItem(change.currentValue);
        auto* file = new QTableWidgetItem(change.fileValue);
        for (auto* item : {setting, current, file}) {
            item->setFlags(Qt::ItemIsEnabled);
        }

        m_table->setItem(row, ApplyColumn, apply);
        m_table->setItem(row, SettingColumn, setting);
        m_table->setItem(row, CurrentColumn, current);
        m_table->setItem(row, FileColumn, file);
    }
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(SettingColumn, QHeaderView::Stretch);
    m_table->resizeRowsToContents();

    auto* selectAll = new QPushButton(tr("Select all"), this);
    auto* selectNone = new QPushButton(tr("Select none"), this);
    connect(selectAll, &QPushButton::clicked, this, [this]() { setAllChecked(true); });
    connect(selectNone, &QPushButton::clicked, this, [this]() { setAllChecked(false); });

    auto* selectionButtons = new QHBoxLayout();
    selectionButtons->addWidget(selectAll);
    selectionButtons->addWidget(selectNone);
    selectionButtons->addStretch();

    // Everything the file stores is either offered above or named here, so that nothing is skipped silently
    auto* notOffered = new QLabel(tr("Also stored in the file, but not offered here because they describe the file "
                                     "itself or a screen position:\n• %1")
                                      .arg(DatabaseSettingsDiff::notOffered().join("\n• ")),
                                  this);
    notOffered->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(this);
    auto* importButton = buttons->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    importButton->setDefault(true);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addLayout(selectionButtons);
    layout->addWidget(notOffered);
    layout->addWidget(buttons);

    resize(860, 560);
}

QList<DatabaseSettingChange> ImportSettingsDialog::chosenChanges() const
{
    QList<DatabaseSettingChange> chosen;
    for (int row = 0; row < m_changes.size(); ++row) {
        if (m_table->item(row, ApplyColumn)->checkState() == Qt::Checked) {
            chosen << m_changes.at(row);
        }
    }
    return chosen;
}

void ImportSettingsDialog::setAllChecked(bool checked)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->item(row, ApplyColumn)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}
