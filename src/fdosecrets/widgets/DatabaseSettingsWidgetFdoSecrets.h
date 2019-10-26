/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H
#define KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H

#include <QScopedPointer>
#include <QSharedPointer>
#include <QWidget>

namespace Ui
{
    class DatabaseSettingsWidgetFdoSecrets;
}

class Database;
class DatabaseSettingsWidgetFdoSecrets : public QWidget
{
    Q_OBJECT
public:
    explicit DatabaseSettingsWidgetFdoSecrets(QWidget* parent = nullptr);
    ~DatabaseSettingsWidgetFdoSecrets() override;

    void loadSettings(QSharedPointer<Database> db);
    void saveSettings();

private:
    void settingsWarning();

private:
    QScopedPointer<Ui::DatabaseSettingsWidgetFdoSecrets> m_ui;

    QSharedPointer<Database> m_db;

    class GroupModelNoRecycle;
    QScopedPointer<GroupModelNoRecycle> m_model;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETFDOSECRETS_H
