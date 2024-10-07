/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_GLOBALTOTPDIALOG_H
#define KEEPASSXC_GLOBALTOTPDIALOG_H

#include "core/Database.h"
#include "core/Entry.h"
#include "gui/DatabaseWidget.h"

namespace Ui
{
    class GlobalTotpDialog;
}

class GlobalTotpDialog : public QWidget
{
    Q_OBJECT

public:
    explicit GlobalTotpDialog(QWidget* parent = nullptr);
    ~GlobalTotpDialog() override;

    void setDatabaseWidget(DatabaseWidget* databaseWidget);

signals:
    void closed();

private Q_SLOTS:
    void updateCounter();
    void updateTable();
    void copyToClipboard();

private:
    QScopedPointer<Ui::GlobalTotpDialog> m_ui;

    QPointer<DatabaseWidget> m_databaseWidget;
    QList<Entry*> m_entries;
    QTimer m_totpUpdateTimer;
};

#endif // KEEPASSXC_GLOBALTOTPDIALOG_H
