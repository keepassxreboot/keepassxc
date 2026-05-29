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

#ifndef KEEPASSXC_DROPBOXSYNCPROVIDER_H
#define KEEPASSXC_DROPBOXSYNCPROVIDER_H

#include "RemoteSyncProvider.h"

#include <QAtomicInt>
#include <QMutex>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
struct DropboxSyncParams;

class DropboxSyncProvider : public RemoteSyncProvider
{
    Q_OBJECT

public:
    explicit DropboxSyncProvider(QObject* parent = nullptr);
    ~DropboxSyncProvider() override;

    // Provider identity, single source of truth referenced by the factory,
    // the settings page, and this provider's own methods.
    static const QString Type; // config "type" tag
    static const QString DisplayName; // untranslated brand name, tr()'d at the UI call site

    // Persisted config keys, shared by the settings page (writer) and
    // buildParamsFromConfig/persist (reader). See RemoteSyncConfigKeys for keys
    // common to all providers.
    static const QString AccessToken;
    static const QString RefreshToken;
    static const QString ExpiresAt;
    static const QString AppKey;

    RemoteHandler::RemoteResult download(const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult upload(const QString& filePath, const RemoteSyncParams* params) override;
    RemoteHandler::RemoteResult refreshAuth(const RemoteSyncParams* params) override;
    void abort() override;

    // RemoteSyncProvider abstraction overrides
    QString displayName() const override;
    RemoteSyncParams* createParams() const override;
    RemoteSyncParams* buildParamsFromConfig(const QJsonObject& config) const override;
    bool applyRefreshedTokens(const QString& stdOutput, RemoteSyncParams* params) override;
    ErrorKind classifyError(const QString& errorMessage) const override;
    bool isAuthorized(const QJsonObject& config) const override;
    void persistRefreshedTokens(const QString& stdOutput, RemoteSettings* settings) const override;

    // Revoke tokens with Dropbox. Best-effort: always returns success
    // (local cleanup is caller's responsibility regardless of revocation outcome).
    // Stays on the provider rather than DropboxLoginFlow because it is a
    // post-session token operation, not part of the login state machine.
    virtual RemoteHandler::RemoteResult revokeToken(const DropboxSyncParams* params);

    // Inject a QNetworkAccessManager for testing (mock QNAM whose post()
    // returns MockNetworkReply). If set, this QNAM is used instead of
    // creating our own. Caller retains ownership: the injected NAM must
    // outlive this object, is never delete-d or reparented by the setter,
    // and calling with nullptr does not free a previously-set NAM.
    void setNetworkAccessManager(QNetworkAccessManager* nam);

    Q_DISABLE_COPY(DropboxSyncProvider)

private:
    // Lazy-construct the QNetworkAccessManager unless one was injected.
    void ensureNam();

    QNetworkAccessManager* m_nam = nullptr;

    static constexpr int MaxDatabaseSize = 256 * 1024 * 1024; // 256 MB sanity limit
    static constexpr int TokenRefreshBufferSecs = 600; // 10-minute proactive refresh

    QString m_lastRev; // Rev from last download, used for upload mode:update
    QNetworkReply* m_activeReply = nullptr; // For abort support
    mutable QMutex m_replyMutex; // Protects m_activeReply across threads
    QAtomicInt m_abortFlag; // Atomic flag checked by HttpRetryHelper between retries
};

#endif // KEEPASSXC_DROPBOXSYNCPROVIDER_H
