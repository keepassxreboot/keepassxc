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

#include "RemoteSettingsCustomDataHandler.h"

#include "core/Metadata.h"
#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <utility>

RemoteSettingsCustomDataHandler::RemoteSettingsCustomDataHandler(QObject* parent, QSharedPointer<Database> db)
    : QObject(parent)
    , m_db(std::move(db))
{
    QString data = m_db->metadata()->customData()->value(CustomData::RemoteProgramSettings);
    QJsonDocument configAsJson = QJsonDocument::fromJson(data.toUtf8());

    QList<RemoteSettings*> typedList;
    foreach (auto variant, configAsJson.array()) {
        auto* remoteProgramParams = new RemoteSettings();
        remoteProgramParams->fromConfig(variant.toObject());
        typedList << remoteProgramParams;
    }
    this->m_lastRemoteProgramEntries = typedList;
}

RemoteSettingsCustomDataHandler::~RemoteSettingsCustomDataHandler() = default;

QList<RemoteSettings*> RemoteSettingsCustomDataHandler::getRemoteProgramEntries()
{
    return this->m_lastRemoteProgramEntries;
}

void RemoteSettingsCustomDataHandler::addRemoteSettingsEntry(RemoteSettings* newRemoteSettings)
{
    QList<RemoteSettings*> toRemove;
    foreach (auto* remoteSettings, m_lastRemoteProgramEntries) {
        if (newRemoteSettings->getName() == remoteSettings->getName()) {
            toRemove << remoteSettings;
        }
    }

    if (toRemove.isEmpty()) {
        m_lastRemoteProgramEntries.append(newRemoteSettings);
    } else if (toRemove.size() == 1) {
        int replaceAt = m_lastRemoteProgramEntries.indexOf(toRemove.at(0));
        m_lastRemoteProgramEntries.replace(replaceAt, newRemoteSettings);
    } else {
        foreach (auto* removeSetting, toRemove) {
            m_lastRemoteProgramEntries.removeOne(removeSetting);
        }
        m_lastRemoteProgramEntries.append(newRemoteSettings);
    }
}

void RemoteSettingsCustomDataHandler::removeRemoteSettingsEntry(const QString& name)
{
    QList<RemoteSettings*> toRemove;
    foreach (auto* remoteSettings, m_lastRemoteProgramEntries) {
        if (name == remoteSettings->getName()) {
            toRemove << remoteSettings;
        }
    }
    foreach (auto* removeSetting, toRemove) {
        m_lastRemoteProgramEntries.removeOne(removeSetting);
    }
}

RemoteSettings* RemoteSettingsCustomDataHandler::getRemoteSettingsEntry(const QString& name)
{
    foreach (auto* remoteSettings, m_lastRemoteProgramEntries) {
        if (name == remoteSettings->getName()) {
            return remoteSettings;
        }
    }
    return nullptr;
}

void RemoteSettingsCustomDataHandler::syncConfig()
{
    QJsonArray lastRemoteProgramEntriesConfig;
    foreach (RemoteSettings* remoteSettings, m_lastRemoteProgramEntries) {
        lastRemoteProgramEntriesConfig << remoteSettings->toConfig();
    }

    QByteArray configAsJson = QJsonDocument(lastRemoteProgramEntriesConfig).toJson(QJsonDocument::Compact);
    auto previousConfig = m_db->metadata()->customData()->value(CustomData::RemoteProgramSettings);
    if (configAsJson != previousConfig) {
        m_db->metadata()->customData()->set(CustomData::RemoteProgramSettings, configAsJson);
        m_db->markAsModified();
    }
}
