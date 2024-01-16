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

#ifndef KEEPASSX_MERGEDIALOG_H
#define KEEPASSX_MERGEDIALOG_H

#include "core/Merger.h"

#include <QDialog>
#include <QMenu>

namespace Ui
{
    class MergeDialog;
}

class Database;

class MergeDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Merge source into copy of target and display changes.
     * On user confirmation, merge source into target.
     */
    explicit MergeDialog(QSharedPointer<Database> source, QSharedPointer<Database> target, QWidget* parent = nullptr);
    /**
     * Display given changes.
     */
    explicit MergeDialog(const Merger::ChangeList& changes, QWidget* parent = nullptr);
    ~MergeDialog() override;

signals:
    /**
     * Signal will be emitted when a normal merge operation has been performed.
     *
     * @param databaseChanged True if the target database was changed due to the merge
     */
    void databaseMerged(bool databaseChanged);
    /**
     * Signal will be emitted when a merge has been performed, but the previously displayed changes differ from
     * the ones that were actually performed.
     *
     * @param actualChanges Actual changes that have been merged
     * @param expectedChanges Changes which were displayed to the user
     */
    void databaseModifiedMerge(const Merger::ChangeList& actualChanges, const Merger::ChangeList& expectedChanges);

private slots:
    void performMerge();
    void abortMerge();
    void refreshMergeChanges();

private:
    QSharedPointer<Database> createTemporaryTargetDatabase();

    enum class MergeDialogColumns
    {
        Group,
        Title,
        Uuid,
        Type,
        Details
    };
    static QVector<MergeDialogColumns> columns();
    static int columnIndex(MergeDialogColumns column);
    static QString columnName(MergeDialogColumns column);
    static QString cellValue(const Merger::Change& change, MergeDialogColumns column);
    static bool isColumnHiddenByDefault(MergeDialogColumns column);

    void calculateChanges();
    void setupChangeTable();
    void setupHeaderContextMenu();
    void updateChangeTable();

    QScopedPointer<Ui::MergeDialog> m_ui;
    QScopedPointer<QMenu> m_headerContextMenu;

    Merger::ChangeList m_changes;
    QSharedPointer<Database> m_sourceDatabase;
    QSharedPointer<Database> m_targetDatabase;
};

#endif // KEEPASSX_MERGEDIALOG_H
