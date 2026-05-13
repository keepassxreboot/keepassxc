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

    void addRemoteParams(RemoteParams params);
    void removeRemoteParams(const QString& name);
    // Returns a non-owning pointer into m_remoteParams. The pointer is stable while
    // m_remoteParams is not mutated; addRemoteParams may rehash and invalidate it
    // (Qt 6 QHash is open-addressed). const_cast in the impl exposes a mutable
    // pointer from a const accessor to keep existing call sites unchanged.
    RemoteParams* getRemoteParams(const QString& name) const;
    QList<RemoteParams*> getAllRemoteParams() const;

    // Generic provider-config API. Lookup is linear over m_providerConfigs;
    // (type, name) is the unique key.

    // Get the persisted JSON config for (type, name). Returns an empty
    // object if no entry exists.
    QJsonObject getProviderConfig(const QString& type, const QString& name) const;
    // Persist the JSON config under (type, name); replaces any existing entry.
    void setProviderConfig(const QString& type, const QString& name, const QJsonObject& config);
    // Remove the persisted entry under (type, name); no-op if not present.
    void removeProviderConfig(const QString& type, const QString& name);

    // Active-provider accessors. Mutating setter trips the touched flag so
    // toConfig switches from raw-array to wrapped-object shape.

    // Returns the type-tag of the currently active provider (e.g. "dropbox").
    // When not explicitly set, defaults lazily to the first persisted entry
    // for which the provider reports isAuthorized().
    QString activeProvider() const;
    // Set the active provider type-tag. Trips the touched flag so subsequent
    // saves use the wrapped-object on-disk shape.
    void setActiveProvider(const QString& type);

    void loadSettings();
    void saveSettings() const;

    // True iff this database has any sync configured: a Script Sync entry
    // in m_remoteParams, OR an active Cloud Sync provider, OR any provider
    // config that represents an authorized state. Used to gate the
    // change-key syncPreviousKey snapshot -- there is no reason to retain
    // the old composite key in memory for users who don't sync.
    bool hasAnySync() const;

private:
    void fromConfig(const QString& data);
    QString toConfig() const;

    QHash<QString, RemoteParams> m_remoteParams;
    QList<QJsonObject> m_providerConfigs; // Insertion-ordered; preserves disk order.
    QString m_activeProvider; // Empty until setActiveProvider or lazy-default.
    bool m_activeProviderTouched = false; // When false, toConfig emits raw-array shape; when true, wrapped-object.
    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_REMOTESETTINGS_H
