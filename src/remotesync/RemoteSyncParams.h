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

#ifndef KEEPASSXC_REMOTESYNCPARAMS_H
#define KEEPASSXC_REMOTESYNCPARAMS_H

#include <QDateTime>
#include <QString>

struct RemoteParams;

// Persisted config keys common to all cloud providers. Provider-specific keys
// live as static const members on the respective provider class.
namespace RemoteSyncConfigKeys
{
    inline const QString Type = QStringLiteral("type");
    inline const QString Name = QStringLiteral("name");
    inline const QString RemotePath = QStringLiteral("remotePath");
    inline const QString SyncOnSave = QStringLiteral("syncOnSave");
    inline const QString SyncOnOpen = QStringLiteral("syncOnOpen");
} // namespace RemoteSyncConfigKeys

// Default network timeouts for the cloud-sync providers (Dropbox/Nextcloud),
// their login flows and the cloud settings UI. The command provider has its
// own process timeouts and does not use these.
namespace CloudSyncDefaults
{
    constexpr int NetworkTimeoutMsec = 30000; // Default network/auth timeout (30s).
} // namespace CloudSyncDefaults

struct RemoteSyncParams
{
    QString type; // "command", "dropbox", etc.
    QString name; // User-visible name

    virtual ~RemoteSyncParams() = default;
};

struct CommandSyncParams : public RemoteSyncParams
{
    CommandSyncParams() = default;
    // Convert to/from the legacy Script-Sync RemoteParams (gui/remote).
    explicit CommandSyncParams(const RemoteParams& params);
    RemoteParams toRemoteParams() const;

    QString downloadCommand;
    QString downloadInput;
    int downloadTimeoutMsec = 10000;
    QString uploadCommand;
    QString uploadInput;
    int uploadTimeoutMsec = 10000;
};

struct DropboxSyncParams : public RemoteSyncParams
{
    QString accessToken; // OAuth2 bearer token (short-lived, ~4 hours)
    QString refreshToken; // Long-lived refresh token (does not expire unless revoked)
    QDateTime expiresAt; // UTC time when accessToken expires
    QString appKey; // User's Dropbox App Key (client_id for PKCE)
    QString remotePath; // e.g., "/Apps/KeePassXC/passwords.kdbx"
    int timeoutMsec = CloudSyncDefaults::NetworkTimeoutMsec;
};

struct NextcloudSyncParams : public RemoteSyncParams
{
    QString serverBaseUrl; // Canonicalized: no trailing slash, scheme defaulted to https://, subpath preserved
    QString remotePath; // NFC-normalized at save; e.g. "/Passwords/db.kdbx" -- must start with '/'
    QString loginName; // Nextcloud account login; populated by Login Flow v2 or paste fallback
    QString appPassword; // Basic-auth password; populated by Login Flow v2 or paste fallback
    int timeoutMsec = CloudSyncDefaults::NetworkTimeoutMsec;
};

#endif // KEEPASSXC_REMOTESYNCPARAMS_H
