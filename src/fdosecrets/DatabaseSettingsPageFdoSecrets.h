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

#ifndef KEEPASSXC_DATABASESETTINGSPAGEFDOSECRETS_H
#define KEEPASSXC_DATABASESETTINGSPAGEFDOSECRETS_H

#include "gui/dbsettings/DatabaseSettingsDialog.h"

class DatabaseSettingsPageFdoSecrets : public IDatabaseSettingsPage
{
    Q_DISABLE_COPY(DatabaseSettingsPageFdoSecrets)
public:
    DatabaseSettingsPageFdoSecrets() = default;

    QString name() override;
    QIcon icon() override;
    QWidget* createWidget() override;
    void loadSettings(QWidget* widget, QSharedPointer<Database> db) override;
    void saveSettings(QWidget* widget) override;
};

#endif // KEEPASSXC_DATABASESETTINGSPAGEFDOSECRETS_H
