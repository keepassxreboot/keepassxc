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

#include "RemoteSettings.h"

#include "core/Database.h"
#include "core/Metadata.h"
#include "remotesync/RemoteSyncProvider.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>

RemoteSettings::RemoteSettings(const QSharedPointer<Database>& db, QObject* parent)
    : QObject(parent)
{
    setDatabase(db);
}

void RemoteSettings::setDatabase(const QSharedPointer<Database>& db)
{
    m_remoteParams.clear();
    m_cloudConfig = QJsonObject();
    m_db = db;
    loadSettings();
}

void RemoteSettings::addRemoteParams(RemoteParams params)
{
    if (params.name.isEmpty()) {
        qWarning() << "RemoteSettings::addRemoteParams: Remote parameters name is empty";
        return;
    }
    m_remoteParams.insert(params.name, std::move(params));
}

void RemoteSettings::removeRemoteParams(const QString& name)
{
    m_remoteParams.remove(name);
}

RemoteParams* RemoteSettings::getRemoteParams(const QString& name) const
{
    auto it = m_remoteParams.constFind(name);
    if (it == m_remoteParams.constEnd()) {
        return nullptr;
    }
    // Logical-const accessor; callers expect a mutable pointer.
    return const_cast<RemoteParams*>(&it.value());
}

QList<RemoteParams*> RemoteSettings::getAllRemoteParams() const
{
    QList<RemoteParams*> result;
    result.reserve(m_remoteParams.size());
    for (auto it = m_remoteParams.constBegin(); it != m_remoteParams.constEnd(); ++it) {
        // See getRemoteParams above for the const_cast rationale.
        result.append(const_cast<RemoteParams*>(&it.value()));
    }
    return result;
}

QJsonObject RemoteSettings::cloudSyncConfig() const
{
    return m_cloudConfig;
}

void RemoteSettings::setCloudSyncConfig(const QJsonObject& config)
{
    m_cloudConfig = config;
}

void RemoteSettings::clearCloudSyncConfig()
{
    m_cloudConfig = QJsonObject();
}

QString RemoteSettings::activeProvider() const
{
    return m_cloudConfig.value(QStringLiteral("type")).toString();
}

void RemoteSettings::loadSettings()
{
    if (m_db) {
        fromConfig(m_db->metadata()->customData()->value(CustomData::RemoteProgramSettings));
        fromCloudConfig(m_db->metadata()->customData()->value(CustomData::CloudSyncSettings));
    }
}

void RemoteSettings::saveSettings() const
{
    if (!m_db) {
        return;
    }
    auto* cd = m_db->metadata()->customData();
    cd->set(CustomData::RemoteProgramSettings, toConfig());
    if (m_cloudConfig.isEmpty()) {
        // Don't leave a stale "{}" key on disk when cloud sync is cleared.
        if (cd->contains(CustomData::CloudSyncSettings)) {
            cd->remove(CustomData::CloudSyncSettings);
        }
    } else {
        cd->set(CustomData::CloudSyncSettings, toCloudConfig());
    }
}

bool RemoteSettings::hasAnySync() const
{
    if (!m_remoteParams.isEmpty()) {
        return true;
    }
    if (m_cloudConfig.isEmpty()) {
        return false;
    }
    const QString type = activeProvider();
    QScopedPointer<RemoteSyncProvider> provider(RemoteSyncProvider::create(type, nullptr));
    return provider && provider->isAuthorized(m_cloudConfig);
}

QString RemoteSettings::toConfig() const
{
    // Pure 1.0 shape: flat array of command-sync entries, no "type" field.
    // Byte-identical to what 1.0 emits for the same input -- this is the
    // contract that keeps KPXC_REMOTE_SYNC_SETTINGS opaque to downgrade.
    QJsonArray arr;
    for (auto it = m_remoteParams.constBegin(); it != m_remoteParams.constEnd(); ++it) {
        const RemoteParams& params = it.value();
        QJsonObject object;
        object[QStringLiteral("name")] = params.name;
        object[QStringLiteral("downloadCommand")] = params.downloadCommand;
        object[QStringLiteral("downloadCommandInput")] = params.downloadInput;
        object[QStringLiteral("downloadTimeoutMsec")] = params.downloadTimeoutMsec;
        object[QStringLiteral("uploadCommand")] = params.uploadCommand;
        object[QStringLiteral("uploadCommandInput")] = params.uploadInput;
        object[QStringLiteral("uploadTimeoutMsec")] = params.uploadTimeoutMsec;
        arr << object;
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void RemoteSettings::fromConfig(const QString& data)
{
    m_remoteParams.clear();

    QJsonDocument json = QJsonDocument::fromJson(data.toUtf8());
    for (const auto& item : json.array().toVariantList()) {
        auto itemMap = item.toMap();
        RemoteParams params;
        params.name = itemMap[QStringLiteral("name")].toString();
        params.downloadCommand = itemMap[QStringLiteral("downloadCommand")].toString();
        params.downloadInput = itemMap[QStringLiteral("downloadCommandInput")].toString();
        params.downloadTimeoutMsec = itemMap.value(QStringLiteral("downloadTimeoutMsec"), 10000).toInt();
        params.uploadCommand = itemMap[QStringLiteral("uploadCommand")].toString();
        params.uploadInput = itemMap[QStringLiteral("uploadCommandInput")].toString();
        params.uploadTimeoutMsec = itemMap.value(QStringLiteral("uploadTimeoutMsec"), 10000).toInt();

        m_remoteParams.insert(params.name, std::move(params));
    }
}

QString RemoteSettings::toCloudConfig() const
{
    if (m_cloudConfig.isEmpty()) {
        return QString();
    }
    return QString::fromUtf8(QJsonDocument(m_cloudConfig).toJson(QJsonDocument::Compact));
}

void RemoteSettings::fromCloudConfig(const QString& data)
{
    m_cloudConfig = QJsonObject();
    if (data.isEmpty()) {
        return;
    }
    QJsonDocument json = QJsonDocument::fromJson(data.toUtf8());
    if (json.isObject()) {
        m_cloudConfig = json.object();
    }
}
