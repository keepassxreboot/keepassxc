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
    m_providerConfigs.clear();
    m_activeProvider.clear();
    m_activeProviderTouched = false;
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

void RemoteSettings::loadSettings()
{
    if (m_db) {
        fromConfig(m_db->metadata()->customData()->value(CustomData::RemoteProgramSettings));
    }
}

void RemoteSettings::saveSettings() const
{
    if (m_db) {
        const QString out = toConfig();
        m_db->metadata()->customData()->set(CustomData::RemoteProgramSettings, out);
    }
}

bool RemoteSettings::hasAnySync() const
{
    if (!m_remoteParams.isEmpty()) {
        return true;
    }
    if (!m_activeProvider.isEmpty()) {
        return true;
    }
    // Defensive: even with no explicit active provider, an authorized
    // provider config means sync is effectively configured -- fromConfig's
    // lazy default would adopt it on the next load, so a change-key here
    // still needs the snapshot.
    for (const auto& cfg : m_providerConfigs) {
        const QString type = cfg.value(QStringLiteral("type")).toString();
        QScopedPointer<RemoteSyncProvider> provider(RemoteSyncProvider::create(type, nullptr));
        if (provider && provider->isAuthorized(cfg)) {
            return true;
        }
    }
    return false;
}

QString RemoteSettings::toConfig() const
{
    QJsonArray providers;
    for (auto it = m_remoteParams.constBegin(); it != m_remoteParams.constEnd(); ++it) {
        const RemoteParams& params = it.value();
        QJsonObject object;
        object[QStringLiteral("type")] = QStringLiteral("command");
        object[QStringLiteral("name")] = params.name;
        object[QStringLiteral("downloadCommand")] = params.downloadCommand;
        object[QStringLiteral("downloadCommandInput")] = params.downloadInput;
        object[QStringLiteral("downloadTimeoutMsec")] = params.downloadTimeoutMsec;
        object[QStringLiteral("uploadCommand")] = params.uploadCommand;
        object[QStringLiteral("uploadCommandInput")] = params.uploadInput;
        object[QStringLiteral("uploadTimeoutMsec")] = params.uploadTimeoutMsec;
        providers << object;
    }
    for (const auto& providerConfig : m_providerConfigs) {
        providers << providerConfig;
    }

    // If nothing in this session touched cloud-sync state, emit the
    // raw-array shape so databases that never engage with cloud sync
    // round-trip to byte-identical output.
    if (!m_activeProviderTouched) {
        return QString::fromUtf8(QJsonDocument(providers).toJson(QJsonDocument::Compact));
    }

    QJsonObject wrapper;
    wrapper[QStringLiteral("activeProvider")] = m_activeProvider;
    wrapper[QStringLiteral("providers")] = providers;
    return QString::fromUtf8(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
}

void RemoteSettings::fromConfig(const QString& data)
{
    m_remoteParams.clear();
    m_providerConfigs.clear();
    m_activeProvider.clear();
    m_activeProviderTouched = false;

    QJsonDocument json = QJsonDocument::fromJson(data.toUtf8());

    QJsonArray providers;
    bool wrappedShape = false;

    if (json.isArray()) {
        // Raw-array shape -- the entire document is the providers list.
        providers = json.array();
    } else if (json.isObject()) {
        // Wrapped-object shape -- {"activeProvider":"...","providers":[...]}.
        wrappedShape = true;
        QJsonObject wrapper = json.object();
        m_activeProvider = wrapper.value(QStringLiteral("activeProvider")).toString();
        providers = wrapper.value(QStringLiteral("providers")).toArray();
    }

    for (const auto& item : providers) {
        QJsonObject obj = item.toObject();

        // Read type field, default to "command" for backward compatibility.
        QString type = obj.value(QStringLiteral("type")).toString(QStringLiteral("command"));

        if (type == QStringLiteral("command") || type.isEmpty()) {
            auto itemMap = item.toVariant().toMap();
            RemoteParams params;
            params.name = itemMap[QStringLiteral("name")].toString();
            params.downloadCommand = itemMap[QStringLiteral("downloadCommand")].toString();
            params.downloadInput = itemMap[QStringLiteral("downloadCommandInput")].toString();
            params.downloadTimeoutMsec = itemMap.value(QStringLiteral("downloadTimeoutMsec"), 10000).toInt();
            params.uploadCommand = itemMap[QStringLiteral("uploadCommand")].toString();
            params.uploadInput = itemMap[QStringLiteral("uploadCommandInput")].toString();
            params.uploadTimeoutMsec = itemMap.value(QStringLiteral("uploadTimeoutMsec"), 10000).toInt();

            m_remoteParams.insert(params.name, std::move(params));
        } else {
            // Generic provider config (every non-command type) stored verbatim
            // so an older binary loading a future provider's entries round-trips
            // them unchanged.
            m_providerConfigs.append(obj);
        }
    }

    // Raw-array shape has no activeProvider field; adopt the first authorized
    // config as a lazy default. Inferred values do not trip
    // m_activeProviderTouched so the round-trip stays in raw-array shape.
    if (!wrappedShape) {
        for (const auto& cfg : m_providerConfigs) {
            const QString type = cfg.value(QStringLiteral("type")).toString();
            QScopedPointer<RemoteSyncProvider> provider(RemoteSyncProvider::create(type, nullptr));
            if (provider && provider->isAuthorized(cfg)) {
                m_activeProvider = type;
                break;
            }
        }
    }
}

QJsonObject RemoteSettings::getProviderConfig(const QString& type, const QString& name) const
{
    for (const auto& cfg : m_providerConfigs) {
        if (cfg.value(QStringLiteral("type")).toString() == type
            && cfg.value(QStringLiteral("name")).toString() == name) {
            return cfg;
        }
    }
    return QJsonObject{};
}

void RemoteSettings::setProviderConfig(const QString& type, const QString& name, const QJsonObject& config)
{
    Q_ASSERT(!type.isEmpty() && !name.isEmpty());

    for (int i = 0; i < m_providerConfigs.size(); ++i) {
        const auto& existing = m_providerConfigs.at(i);
        if (existing.value(QStringLiteral("type")).toString() == type
            && existing.value(QStringLiteral("name")).toString() == name) {
            m_providerConfigs[i] = config;
            m_activeProviderTouched = true;
            return;
        }
    }
    m_providerConfigs.append(config);
    m_activeProviderTouched = true;
}

void RemoteSettings::removeProviderConfig(const QString& type, const QString& name)
{
    for (int i = 0; i < m_providerConfigs.size(); ++i) {
        const auto& existing = m_providerConfigs.at(i);
        if (existing.value(QStringLiteral("type")).toString() == type
            && existing.value(QStringLiteral("name")).toString() == name) {
            m_providerConfigs.removeAt(i);
            // Clear active if it pointed at the removed entry; downstream consumers
            // (isCloudSyncAuthorized, auto-select) expect activeProvider to name a
            // real config.
            if (m_activeProvider == type) {
                m_activeProvider.clear();
            }
            m_activeProviderTouched = true;
            return;
        }
    }
}

QString RemoteSettings::activeProvider() const
{
    return m_activeProvider;
}

void RemoteSettings::setActiveProvider(const QString& type)
{
    m_activeProvider = type;
    m_activeProviderTouched = true;
}
