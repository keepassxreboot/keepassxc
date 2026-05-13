/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#ifndef KEEPASSX_DATABASESETTINGSWIDGETCLOUDSYNC_H
#define KEEPASSX_DATABASESETTINGSWIDGETCLOUDSYNC_H

#include "CloudSyncPage.h"
#include "RemoteHandler.h"
#include "gui/dbsettings/DatabaseSettingsWidget.h"
#include "remotesync/RemoteSyncProvider.h"

#include <QList>
#include <QPointer>
#include <QScopedPointer>

class RemoteSettings;

namespace Ui
{
    class DatabaseSettingsWidgetCloudSync;
}

// Provider-agnostic parent widget for cloud-sync settings. Holds a list of
// registered CloudSyncPage subclasses produced by
// CloudSyncPage::createBuiltinPages. Drives UI dispatch via the abstract
// CloudSyncPage contract -- never via dynamic_cast or concrete-type
// knowledge.
class DatabaseSettingsWidgetCloudSync : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidgetCloudSync(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingsWidgetCloudSync);
    ~DatabaseSettingsWidgetCloudSync() override;

signals:
    void cloudSyncTriggered();
    void settingsModified();

public slots:
    void initialize() override;
    void uninitialize() override;
    bool saveSettings() override;

private slots:
    void onProviderChanged(int index);
    void onPageShowMessage(const QString& text, int messageType, bool disableAutoHide);
    void onPageHideMessage();
    void onPageRequestSync();

private:
    bool hasScriptSyncConfig() const;
    CloudSyncPage* activePage() const;
    void registerPage(CloudSyncPage* page);
    void updateSize();

    QScopedPointer<RemoteSettings> m_remoteSettings;
    const QScopedPointer<Ui::DatabaseSettingsWidgetCloudSync> m_ui;
    QList<CloudSyncPage*> m_pages; // Insertion-ordered; index parallels providerComboBox.
    bool m_lockedByScriptSync{false}; // Set by initialize() when Script Sync is configured -- gates save.
};

#endif // KEEPASSX_DATABASESETTINGSWIDGETCLOUDSYNC_H
