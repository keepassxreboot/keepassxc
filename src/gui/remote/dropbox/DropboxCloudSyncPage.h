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

#ifndef KEEPASSX_DROPBOXCLOUDSYNCPAGE_H
#define KEEPASSX_DROPBOXCLOUDSYNCPAGE_H

#include "gui/remote/CloudSyncPage.h"
#include "gui/remote/RemoteHandler.h"

#include <QJsonObject>
#include <QPointer>
#include <QScopedPointer>
#include <memory>

class DropboxLoginFlow;
class DropboxSyncProvider;
class RemoteSettings;
struct DropboxSyncParams;

namespace Ui
{
    class DropboxCloudSyncPage;
}

class DropboxCloudSyncPage : public CloudSyncPage
{
    Q_OBJECT

public:
    explicit DropboxCloudSyncPage(QWidget* parent = nullptr);
    ~DropboxCloudSyncPage() override;

    // CloudSyncPage contract ----------------------------------------------
    void setProvider(RemoteSyncProvider* provider) override;
    QString providerType() const override;
    QString providerDisplayName() const override;
    void loadFromConfig(const QJsonObject& config) override;
    QJsonObject saveToConfig() const override;
    bool isModified() const override;
    void setRemoteSettings(RemoteSettings* settings) override;
    void setMutualExclusivityWarning(bool active) override;

    static const QString ConfigName;

    // Test seam: inject a DropboxLoginFlow (typically a MockDropboxLoginFlow)
    // BEFORE the first Authorize click. Production lazy-constructs the default
    // on first click via onAuthorizeClicked. Page takes ownership.
    void setLoginFlowForTest(DropboxLoginFlow* flow);

private slots:
    void onAuthorizeClicked();
    void onTestConnectionClicked();
    void onRemoveClicked();
    void onRevokeClicked();
    void onSubmitManualCode();
    void onCancelManualCode();
    void onTriggerSyncClicked();

    // 4 DropboxLoginFlow signals -- mirror NextcloudCloudSyncPage's slot set.
    void onAuthorizationManualFallback(const QString& codeVerifier);
    void onAuthorizationCompleted(const QString& accessToken, const QString& refreshToken, qint64 expiresAtMs);
    void onAuthorizationFailed(const QString& reason);
    void onAuthorizationCancelled();

private:
    Q_DISABLE_COPY(DropboxCloudSyncPage)

    enum class AuthState
    {
        Idle,
        Authorizing,
        ManualFallback,
        Authorized,
        Revoking
    };

    void updateAuthStatus(AuthState state);

    // Persist freshly-acquired tokens into m_config + RemoteSettings (without
    // saving the database; Apply-button click is the persist gate).
    void mergeAndPersistTokens(const QString& accessToken, const QString& refreshToken, qint64 expiresAtMs);

    // Build a DropboxSyncParams from the current UI fields + cached config.
    // Used by Revoke / Test Connection click handlers.
    std::unique_ptr<DropboxSyncParams> buildDropboxParams() const;

    // Disable / re-enable editable fields during in-flight auth or under the
    // mutual-exclusivity warning.
    void setFieldsEnabled(bool enabled);

    // Lazy-construct m_loginFlow + wire signals. Idempotent (no-op if already
    // constructed). Mirrors NextcloudCloudSyncPage's lazy-construct pattern.
    void ensureLoginFlow();

    QScopedPointer<Ui::DropboxCloudSyncPage> m_ui;
    QScopedPointer<DropboxLoginFlow> m_loginFlow; // lazily instantiated on first Authorize
    QPointer<DropboxSyncProvider> m_dropboxProvider; // Borrowed via setProvider; owned externally.
    QPointer<RemoteSettings> m_remoteSettings; // Borrowed; parent retains ownership.
    QJsonObject m_config; // Current Dropbox config loaded from RemoteSettings.
    AuthState m_authState = AuthState::Idle;
    bool m_modified = false;
    bool m_mutualExclusivityActive = false;
};

#endif // KEEPASSX_DROPBOXCLOUDSYNCPAGE_H
