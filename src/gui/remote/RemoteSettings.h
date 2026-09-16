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

#ifndef KEEPASSXC_REMOTESETTINGS_H
#define KEEPASSXC_REMOTESETTINGS_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>

class Database;

struct RemoteParams
{
    QString name;
    QString downloadCommand;
    QString downloadInput;
    int downloadTimeoutMsec;
    QString uploadCommand;
    QString uploadInput;
    int uploadTimeoutMsec;
};
Q_DECLARE_METATYPE(RemoteParams)

class RemoteSettings : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSettings(const QSharedPointer<Database>& db, QObject* parent = nullptr);
    ~RemoteSettings() override = default;

    void setDatabase(const QSharedPointer<Database>& db);

    // Script Sync (CustomData::RemoteProgramSettings, byte-identical to 1.0).
    void addRemoteParams(RemoteParams params);
    void removeRemoteParams(const QString& name);
    // Returns a non-owning pointer into m_remoteParams. The pointer is stable while
    // m_remoteParams is not mutated; addRemoteParams may rehash and invalidate it
    // (Qt 6 QHash is open-addressed). const_cast in the impl exposes a mutable
    // pointer from a const accessor to keep existing call sites unchanged.
    RemoteParams* getRemoteParams(const QString& name) const;
    QList<RemoteParams*> getAllRemoteParams() const;

    // Cloud Sync (CustomData::CloudSyncSettings, separate key invisible to
    // older builds). Single-provider model: at most one cloud config exists.

    // Returns the persisted cloud-sync config, or an empty object if none.
    QJsonObject cloudSyncConfig() const;
    // Replaces the cloud-sync config in memory; pass an empty object to clear.
    // Persistence happens on saveSettings().
    void setCloudSyncConfig(const QJsonObject& config);
    // Clear the cloud-sync config. Equivalent to setCloudSyncConfig({}).
    void clearCloudSyncConfig();
    // Type tag of the active cloud provider (e.g. "dropbox"), derived from
    // the cloud config's "type" field. Empty when no cloud config is set.
    QString activeProvider() const;

    void loadSettings();
    void saveSettings() const;

    // True iff this database has any sync configured: a Script Sync entry
    // in m_remoteParams, or an authorized cloud-sync provider. Used to gate
    // the change-key syncPreviousKey snapshot -- there is no reason to retain
    // the old composite key in memory for users who don't sync.
    bool hasAnySync() const;

private:
    void fromConfig(const QString& data);
    QString toConfig() const;
    void fromCloudConfig(const QString& data);
    QString toCloudConfig() const;

    QHash<QString, RemoteParams> m_remoteParams;
    QJsonObject m_cloudConfig; // Empty = no cloud sync configured.
    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_REMOTESETTINGS_H
