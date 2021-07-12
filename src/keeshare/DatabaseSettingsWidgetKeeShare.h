/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETKEESHARE_H
#define KEEPASSXC_DATABASESETTINGSWIDGETKEESHARE_H

#include <QPointer>
#include <QWidget>

class Database;

class QStandardItemModel;

namespace Ui
{
    class DatabaseSettingsWidgetKeeShare;
}

class DatabaseSettingsWidgetKeeShare : public QWidget
{
    Q_OBJECT
public:
    explicit DatabaseSettingsWidgetKeeShare(QWidget* parent = nullptr);
    ~DatabaseSettingsWidgetKeeShare();

    void loadSettings(QSharedPointer<Database> db);
    void saveSettings();

private:
    QScopedPointer<Ui::DatabaseSettingsWidgetKeeShare> m_ui;

    QScopedPointer<QStandardItemModel> m_referencesModel;
    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETKEESHARE_H
