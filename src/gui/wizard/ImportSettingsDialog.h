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

#ifndef KEEPASSXC_IMPORTSETTINGSDIALOG_H
#define KEEPASSXC_IMPORTSETTINGSDIALOG_H

#include <QDialog>
#include <QList>

#include "core/DatabaseSettingsDiff.h"

class QTableWidget;

/**
 * Shown when an XML file is imported into a database that is already open. Importing there only adds the
 * folders and entries; this dialog lists every database setting of the file that differs from the open database, so
 * that nothing is dropped without the user knowing. Each setting has a checkbox (unticked) to apply it.
 */
class ImportSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        OpenDatabase, // imported into a database that is open: nothing is ticked, the user's database stays as it is
        NewDatabase // imported into a new database: everything is ticked, the file defines the new database
    };

    explicit ImportSettingsDialog(const QList<DatabaseSettingChange>& changes,
                                  QWidget* parent = nullptr,
                                  Mode mode = Mode::OpenDatabase);
    ~ImportSettingsDialog() override = default;

    // The settings the user ticked
    QList<DatabaseSettingChange> chosenChanges() const;

private slots:
    void setAllChecked(bool checked);

private:
    QList<DatabaseSettingChange> m_changes;
    QTableWidget* m_table = nullptr;
};

#endif // KEEPASSXC_IMPORTSETTINGSDIALOG_H
