/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_DATABASESETTINGSWIDGETREMOTE_H
#define KEEPASSX_DATABASESETTINGSWIDGETREMOTE_H

#include "gui/dbsettings/DatabaseSettingsWidget.h"

#include <QListWidgetItem>
#include <QPointer>

class Database;
class RemoteSettings;

namespace Ui
{
    class DatabaseSettingsWidgetRemote;
}

class DatabaseSettingsWidgetRemote : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidgetRemote(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingsWidgetRemote);
    ~DatabaseSettingsWidgetRemote() override;

public slots:
    void initialize() override;
    void uninitialize() override;
    bool saveSettings() override;

private slots:
    void saveCurrentSettings();
    void removeCurrentSettings();
    void editCurrentSettings();
    void testDownload();

private:
    void updateSettingsList();
    QListWidgetItem* findItemByName(const QString& name);
    void clearFields();

    QScopedPointer<RemoteSettings> m_remoteSettings;
    const QScopedPointer<Ui::DatabaseSettingsWidgetRemote> m_ui;
    bool m_modified = false;
};

#endif // KEEPASSX_DATABASESETTINGSWIDGETREMOTE_H
