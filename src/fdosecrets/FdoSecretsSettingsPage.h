/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_FDOSECRETSSETTINGSPAGE_H
#define KEEPASSXC_FDOSECRETSSETTINGSPAGE_H

#include "gui/ApplicationSettingsWidget.h"
#include "gui/Icons.h"

class DatabaseWidget;
class DatabaseTabWidget;
class FdoSecretsPlugin;

class FdoSecretsSettingsPage : public QObject, public ISettingsPage
{
    Q_OBJECT

public:
    FdoSecretsSettingsPage(FdoSecretsPlugin* plugin, DatabaseTabWidget* dbTabs);
    ~FdoSecretsSettingsPage() override = default;

    QString name() override
    {
        return QObject::tr("Secret Service Integration");
    }

    QIcon icon() override
    {
        return icons()->icon("freedesktop");
    }

    DatabaseTabWidget* dbTabs() const
    {
        return m_dbTabs;
    }

    QWidget* createWidget() override;
    void loadSettings(QWidget* widget) override;
    void saveSettings(QWidget* widget) override;

    void unlockDatabaseInDialog(DatabaseWidget* dbWidget);
    void switchToDatabaseSettings(DatabaseWidget* dbWidget);

signals:
    void requestSwitchToDatabases();

private:
    FdoSecretsPlugin* m_plugin;
    DatabaseTabWidget* m_dbTabs;
};

#endif // KEEPASSXC_FDOSECRETSSETTINGSPAGE_H
