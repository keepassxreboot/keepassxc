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

#ifndef KEEPASSX_NEXTCLOUDCLOUDSYNCPAGE_H
#define KEEPASSX_NEXTCLOUDCLOUDSYNCPAGE_H

#include "gui/remote/CloudSyncPage.h"

#include <QJsonObject>
#include <QPointer>
#include <QScopedPointer>

#include <functional>

class QUrl;

namespace Ui
{
    class NextcloudCloudSyncPage;
}

class NextcloudLoginFlow;
class NextcloudSyncProvider;
class RemoteSettings;
class RemoteSyncProvider;

class NextcloudCloudSyncPage : public CloudSyncPage
{
    Q_OBJECT
    Q_DISABLE_COPY(NextcloudCloudSyncPage)

public:
    explicit NextcloudCloudSyncPage(QWidget* parent = nullptr);
    ~NextcloudCloudSyncPage() override;

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

    // Test seam mirroring NextcloudLoginFlow::setBrowserOpener. Production
    // default invokes QDesktopServices::openUrl. Tests inject a capture
    // lambda to assert on the URL without launching a real browser. Empty
    // std::function is ignored so production never accidentally clears the
    // default lambda.
    using BrowserOpener = std::function<void(const QUrl&)>;
    void setBrowserOpener(BrowserOpener opener);

private slots:
    void onAuthorizeClicked();
    void onAppPasswordAuthorizeClicked();
    void onOpenSecurityClicked();
    void onTestConnectionClicked();
    void onRemoveClicked();
    void onTriggerSyncClicked();

    // 4 of 5 NextcloudLoginFlow signals -- pollingTick is intentionally NOT
    // wired.
    void onLoginInitiated(const QUrl& loginUrl);
    void onLoginCompleted(const QString& loginName, const QString& appPassword);
    void onLoginFailed(const QString& reason);
    void onLoginCancelled();

private:
    enum class AuthState
    {
        Idle,
        Authorizing,
        Authorized,
        Removing
    };

    void updateAuthStatus(AuthState state);
    void setFieldsEnabled(bool enabled);

    // Validate a user-typed server URL via NextcloudSyncProvider::validateServerUrl
    // and dispatch the per-case banner. On Ok: writes the canonical form to
    // canonicalOut and returns true. On Empty/NotSecure/Malformed: emits the
    // appropriate showMessage and returns false (canonicalOut untouched).
    //
    // Single entry point shared by onAuthorize / onAppPasswordAuthorize /
    // onOpenSecurity so all three handlers surface the same dispatch for
    // the same input.
    bool validateAndCanonicalizeServerUrl(const QString& input, QString& canonicalOut);

    QScopedPointer<Ui::NextcloudCloudSyncPage> m_ui;
    QScopedPointer<NextcloudLoginFlow> m_loginFlow; // lazily instantiated on first Authorize
    QPointer<NextcloudSyncProvider> m_nextcloudProvider; // Borrowed via setProvider; owned externally
    QPointer<RemoteSettings> m_remoteSettings; // Borrowed; parent retains ownership
    QJsonObject m_config; // Current Nextcloud config loaded from RemoteSettings
    BrowserOpener m_browserOpener; // Production default = QDesktopServices::openUrl
    AuthState m_authState{AuthState::Idle};
    bool m_modified{false};
    bool m_mutualExclusivityActive{false}; // True when Script Sync is configured -- gates field re-enables
};

#endif // KEEPASSX_NEXTCLOUDCLOUDSYNCPAGE_H
