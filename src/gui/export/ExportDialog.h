/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_EXPORTDIALOG_H
#define KEEPASSXC_EXPORTDIALOG_H

#include "core/Database.h"
#include "gui/DatabaseTabWidget.h"
#include <QDialog>

namespace Ui
{
    class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QSharedPointer<const Database> db, DatabaseTabWidget* parent = nullptr);
    ~ExportDialog() override;

    enum ExportSortingStrategy
    {
        BY_DATABASE_ORDER = 0,
        BY_NAME_ASC = 1,
        BY_NAME_DESC = 2
    };

signals:
    void exportFailed(QString reason);

private slots:
    void exportDatabase();

private:
    QString getStrategyName(ExportSortingStrategy strategy);

    QScopedPointer<Ui::ExportDialog> m_ui;
    QSharedPointer<const Database> m_db;
};

#endif // KEEPASSXC_EXPORTDIALOG_H
