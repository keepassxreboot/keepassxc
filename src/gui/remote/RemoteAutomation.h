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

#ifndef KEEPASSXC_REMOTEAUTOMATION_H
#define KEEPASSXC_REMOTEAUTOMATION_H

#include <QSharedPointer>
#include <QString>

class Database;

/**
 * Per-database configuration and runtime state for automatic remote syncing.
 *
 * The configuration is stored in the database metadata custom data under
 * KPXC_REMOTE_SYNC_AUTOMATION and is marked protected, so a merge never removes it.
 * Note that the merge engine still transfers custom-data values, see issue #14020:
 * full per-client isolation of these settings requires the merge-side fix.
 *
 * The runtime state guarantees that at most one sync per database is in flight.
 * Triggers that arrive while a sync is running are collapsed into a single
 * pending follow-up instead of being queued one by one.
 */
class RemoteAutomation
{
public:
    enum Trigger
    {
        Open,
        Save,
        Close,
    };

    struct Config
    {
        bool enabled = false;
        QString profile;
        bool onOpen = false;
        bool onSave = false;
        bool onClose = false;
    };

    explicit RemoteAutomation(const QSharedPointer<Database>& db = nullptr);

    void setDatabase(const QSharedPointer<Database>& db);

    void load();
    void save() const;

    const Config& config() const;
    void setConfig(const Config& config);
    void disable();

    bool isEnabled() const;
    bool triggerEnabled(Trigger trigger) const;

    /**
     * Decide whether the given trigger starts a sync now.
     *
     * Returns false when automation is disabled, no profile is selected, or a
     * sync is already running. In the latter case the request is remembered as
     * a single pending follow-up (collapsing, not queuing).
     */
    bool requestSync(Trigger trigger);

    void beginSync();
    void endSync();
    bool syncInProgress() const;
    bool hasPendingSync() const;
    void clearPendingSync();

    /**
     * The local file was written since the last successful sync. The close trigger
     * uses this so a manual save followed by closing still reaches the remote copy.
     */
    void markLocalChange();
    void markSynced();
    bool hasUnsyncedChanges() const;

private:
    QSharedPointer<Database> m_db;
    Config m_config;
    bool m_syncInProgress = false;
    bool m_pendingSync = false;
    bool m_unsyncedChanges = false;
};

#endif // KEEPASSXC_REMOTEAUTOMATION_H
