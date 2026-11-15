/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "RemoteAutomation.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Metadata.h"

RemoteAutomation::RemoteAutomation(const QSharedPointer<Database>& db)
    : m_db(db)
{
    load();
}

void RemoteAutomation::setDatabase(const QSharedPointer<Database>& db)
{
    m_db = db;
    load();
}

void RemoteAutomation::load()
{
    m_config = Config();
    if (!m_db || !m_db->metadata() || !m_db->metadata()->customData()) {
        return;
    }

    const auto& customData = m_db->metadata()->customData();
    if (!customData->contains(CustomData::RemoteSyncAutomation)) {
        return;
    }

    const auto document = QJsonDocument::fromJson(customData->value(CustomData::RemoteSyncAutomation).toUtf8());
    if (!document.isObject()) {
        return;
    }

    const auto obj = document.object();
    m_config.enabled = obj.value("enabled").toBool(false);
    m_config.profile = obj.value("profile").toString();
    m_config.onOpen = obj.value("onOpen").toBool(false);
    m_config.onSave = obj.value("onSave").toBool(false);
    m_config.onClose = obj.value("onClose").toBool(false);

    // Automation without a profile is meaningless; keep the state consistent.
    if (m_config.profile.isEmpty()) {
        m_config.enabled = false;
    }
}

void RemoteAutomation::save() const
{
    if (!m_db || !m_db->metadata() || !m_db->metadata()->customData()) {
        return;
    }

    QJsonObject obj;
    obj["enabled"] = m_config.enabled;
    obj["profile"] = m_config.profile;
    obj["onOpen"] = m_config.onOpen;
    obj["onSave"] = m_config.onSave;
    obj["onClose"] = m_config.onClose;

    m_db->metadata()->customData()->set(CustomData::RemoteSyncAutomation,
                                        QString(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    // Custom data changes alone do not flag the database as modified, so make sure
    // the automation settings are actually written to the file.
    m_db->markAsModified();
}

void RemoteAutomation::setConfig(const Config& config)
{
    m_config = config;
    if (m_config.profile.isEmpty()) {
        m_config.enabled = false;
    }
    save();
}

void RemoteAutomation::disable()
{
    m_config = Config();
    save();
}

const RemoteAutomation::Config& RemoteAutomation::config() const
{
    return m_config;
}

bool RemoteAutomation::isEnabled() const
{
    return m_config.enabled && !m_config.profile.isEmpty();
}

bool RemoteAutomation::triggerEnabled(RemoteAutomation::Trigger trigger) const
{
    if (!isEnabled()) {
        return false;
    }

    switch (trigger) {
    case Open:
        return m_config.onOpen;
    case Save:
        return m_config.onSave;
    case Close:
        return m_config.onClose;
    }
    return false;
}

bool RemoteAutomation::requestSync(RemoteAutomation::Trigger trigger)
{
    if (!triggerEnabled(trigger)) {
        return false;
    }

    if (m_syncInProgress) {
        // Collapse concurrent triggers into one follow-up, never a queue.
        m_pendingSync = true;
        return false;
    }

    return true;
}

void RemoteAutomation::beginSync()
{
    m_syncInProgress = true;
}

void RemoteAutomation::endSync()
{
    m_syncInProgress = false;
}

bool RemoteAutomation::syncInProgress() const
{
    return m_syncInProgress;
}

bool RemoteAutomation::hasPendingSync() const
{
    return m_pendingSync;
}

void RemoteAutomation::clearPendingSync()
{
    m_pendingSync = false;
}
