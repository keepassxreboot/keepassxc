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

#include "DropboxCloudSyncPage.h"
#include "ui_DropboxCloudSyncPage.h"

#include "gui/MessageWidget.h"
#include "gui/remote/RemoteSettings.h"
#include "gui/styles/StateColorPalette.h"
#include "remotesync/DropboxLoginFlow.h"
#include "remotesync/DropboxSyncProvider.h"
#include "remotesync/RemoteSyncParams.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QVariant>

const QString DropboxCloudSyncPage::ConfigName = QStringLiteral("dropbox-default");

DropboxCloudSyncPage::DropboxCloudSyncPage(QWidget* parent)
    : CloudSyncPage(parent)
    , m_ui(new Ui::DropboxCloudSyncPage())
{
    m_ui->setupUi(this);
    m_ui->manualCodeWidget->setHidden(true);

    // Click handlers ----------------------------------------------------
    connect(m_ui->authorizeButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onAuthorizeClicked);
    connect(m_ui->testConnectionButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onTestConnectionClicked);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onRemoveClicked);
    connect(m_ui->submitCodeButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onSubmitManualCode);
    connect(m_ui->cancelCodeButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onCancelManualCode);
    connect(m_ui->triggerSyncButton, &QPushButton::clicked, this, &DropboxCloudSyncPage::onTriggerSyncClicked);

    // Modified signal wiring -------------------------------------------
    // Disabling triggerSyncButton when m_modified flips true ensures
    // Sync Now obeys the "click Apply first" banner -- syncing reads
    // from RemoteSettings (persisted), not page state, so an unsaved
    // edit would either no-op or fire stale credentials.
    auto markModified = [this] {
        m_modified = true;
        m_ui->triggerSyncButton->setEnabled(false);
        emit modified();
    };
    connect(m_ui->appKeyEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->remotePathEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->syncOnSaveCheckBox, &QCheckBox::toggled, this, markModified);
    connect(m_ui->syncOnOpenCheckBox, &QCheckBox::toggled, this, markModified);
}

DropboxCloudSyncPage::~DropboxCloudSyncPage() = default;

// ---------------------------------------------------------------------------
// CloudSyncPage contract overrides
// ---------------------------------------------------------------------------

void DropboxCloudSyncPage::setProvider(RemoteSyncProvider* provider)
{
    // Borrowed pointer; cast to concrete type for the Dropbox-specific
    // revokeToken method (not on the abstract base since revocation is an
    // OAuth-specific operation). qobject_cast inside a Dropbox*-named file is
    // the legitimate exemption. The provider's lifetime is owned by the
    // widget that constructed it via RemoteSyncProvider::create.
    m_dropboxProvider = qobject_cast<DropboxSyncProvider*>(provider);
}

QString DropboxCloudSyncPage::providerType() const
{
    return QStringLiteral("dropbox");
}

QString DropboxCloudSyncPage::providerDisplayName() const
{
    return QStringLiteral("Dropbox");
}

void DropboxCloudSyncPage::loadFromConfig(const QJsonObject& config)
{
    // Reset UI to clean state before loading (handles re-entry).
    m_ui->manualCodeWidget->setHidden(true);
    m_ui->manualCodeEdit->clear();
    m_ui->authorizeButton->setHidden(false);
    setFieldsEnabled(true);

    m_config = config;

    // Block signals while populating UI from config to avoid false modified-flag triggers
    const QSignalBlocker appKeyBlocker(m_ui->appKeyEdit);
    const QSignalBlocker remotePathBlocker(m_ui->remotePathEdit);
    const QSignalBlocker syncOnSaveBlocker(m_ui->syncOnSaveCheckBox);
    const QSignalBlocker syncOnOpenBlocker(m_ui->syncOnOpenCheckBox);

    m_ui->appKeyEdit->setText(m_config[QStringLiteral("appKey")].toString());
    m_ui->remotePathEdit->setText(m_config[QStringLiteral("remotePath")].toString());
    m_ui->syncOnSaveCheckBox->setChecked(m_config.value(QStringLiteral("syncOnSave")).toBool(true));
    m_ui->syncOnOpenCheckBox->setChecked(m_config.value(QStringLiteral("syncOnOpen")).toBool(true));

    QString accessToken = m_config[QStringLiteral("accessToken")].toString();
    if (!accessToken.isEmpty()) {
        m_authState = AuthState::Authorized;
    } else {
        m_authState = AuthState::Idle;
    }
    // Reset m_modified BEFORE updateAuthStatus so the Authorized branch's
    // triggerSyncButton gate sees the post-load state (clean, not dirty).
    m_modified = false;
    updateAuthStatus(m_authState);
}

QJsonObject DropboxCloudSyncPage::saveToConfig() const
{
    // Fresh-no-edit fast path: every field empty AND no cached config means
    // the user opened the dialog and never touched anything. Return an empty
    // object so the parent skips persistence entirely.
    if (m_ui->appKeyEdit->text().trimmed().isEmpty() && m_ui->remotePathEdit->text().trimmed().isEmpty()
        && m_config.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject config;
    config[QStringLiteral("type")] = providerType();
    config[QStringLiteral("name")] = ConfigName;
    config[QStringLiteral("appKey")] = m_ui->appKeyEdit->text().trimmed();
    config[QStringLiteral("remotePath")] = m_ui->remotePathEdit->text().trimmed();

    // Preserve token fields from cached config (auth flow sets these, not the UI)
    if (m_config.contains(QStringLiteral("accessToken"))) {
        config[QStringLiteral("accessToken")] = m_config[QStringLiteral("accessToken")];
    }
    if (m_config.contains(QStringLiteral("refreshToken"))) {
        config[QStringLiteral("refreshToken")] = m_config[QStringLiteral("refreshToken")];
    }
    if (m_config.contains(QStringLiteral("expiresAt"))) {
        config[QStringLiteral("expiresAt")] = m_config[QStringLiteral("expiresAt")];
    }

    config[QStringLiteral("syncOnSave")] = m_ui->syncOnSaveCheckBox->isChecked();
    config[QStringLiteral("syncOnOpen")] = m_ui->syncOnOpenCheckBox->isChecked();
    return config;
}

bool DropboxCloudSyncPage::isModified() const
{
    return m_modified;
}

// ---------------------------------------------------------------------------
// Dropbox-specific orchestration
// ---------------------------------------------------------------------------

std::unique_ptr<DropboxSyncParams> DropboxCloudSyncPage::buildDropboxParams() const
{
    auto params = std::make_unique<DropboxSyncParams>();
    params->type = providerType();
    params->name = ConfigName;
    // UI fields override config values (user may have edited them)
    params->appKey = m_ui->appKeyEdit->text().trimmed();
    params->remotePath = m_ui->remotePathEdit->text().trimmed();
    // Token data comes from stored config
    params->accessToken = m_config[QStringLiteral("accessToken")].toString();
    params->refreshToken = m_config[QStringLiteral("refreshToken")].toString();
    params->expiresAt = QDateTime::fromMSecsSinceEpoch(m_config[QStringLiteral("expiresAt")].toVariant().toLongLong());
    params->timeoutMsec = 30000;
    return params;
}

void DropboxCloudSyncPage::setRemoteSettings(RemoteSettings* settings)
{
    m_remoteSettings = settings;
}

void DropboxCloudSyncPage::setMutualExclusivityWarning(bool active)
{
    m_mutualExclusivityActive = active;
    setFieldsEnabled(!active);
}

void DropboxCloudSyncPage::setFieldsEnabled(bool enabled)
{
    m_ui->appKeyEdit->setEnabled(enabled);
    m_ui->remotePathEdit->setEnabled(enabled);
    m_ui->authorizeButton->setEnabled(enabled);
    m_ui->testConnectionButton->setEnabled(enabled);
    m_ui->removeButton->setEnabled(enabled);
    m_ui->syncOnSaveCheckBox->setEnabled(enabled);
    m_ui->syncOnOpenCheckBox->setEnabled(enabled);
}

// ---------------------------------------------------------------------------
// Click handlers
// ---------------------------------------------------------------------------

void DropboxCloudSyncPage::onAuthorizeClicked()
{
    // In Authorized state, the button acts as "Revoke".
    if (m_authState == AuthState::Authorized) {
        onRevokeClicked();
        return;
    }

    // In Authorizing state, the button acts as "Cancel Authorization".
    // DropboxLoginFlow::cancel is idempotent and emits authorizationCancelled
    // exactly once when it stops an active flow -- the slot resets our state.
    if (m_authState == AuthState::Authorizing) {
        if (m_loginFlow) {
            m_loginFlow->cancel();
        }
        return;
    }

    // Validate: app key required.
    const QString appKey = m_ui->appKeyEdit->text().trimmed();
    if (appKey.isEmpty()) {
        emit showMessage(tr("App Key is required for authorization."), MessageWidget::Warning, false);
        return;
    }

    // Lazy-construct the login flow on first Authorize click; reuse on
    // subsequent clicks (startAuthorization is internally cancel-previous,
    // so reuse has equivalent semantics with less destructor churn).
    ensureLoginFlow();

    // Enter Authorizing state (changes button to "Cancel Authorization",
    // disables fields). Keep the Authorize button live so its "Cancel
    // Authorization" label is clickable.
    updateAuthStatus(AuthState::Authorizing);
    setFieldsEnabled(false);
    m_ui->authorizeButton->setEnabled(true);

    // Start the flow. Signal-driven from here -- no nested QEventLoop, no
    // QPointer<self> reentrancy guard needed. The 4 slot wirings in
    // ensureLoginFlow handle every terminal outcome.
    m_loginFlow->startAuthorization(appKey, 30000);

    emit requestAuthorize();
}

void DropboxCloudSyncPage::onRevokeClicked()
{
    if (!m_dropboxProvider) {
        return;
    }

    // Build params from current config for revocation
    auto params = buildDropboxParams();

    // Show revoking feedback
    updateAuthStatus(AuthState::Revoking);
    setFieldsEnabled(false);

    // Best-effort revoke -- ignore result.
    QPointer<DropboxCloudSyncPage> guard(this);
    m_dropboxProvider->revokeToken(params.get());
    if (!guard) {
        return;
    }

    // Clear token fields from cached config
    m_config.remove(QStringLiteral("accessToken"));
    m_config.remove(QStringLiteral("refreshToken"));
    m_config.remove(QStringLiteral("expiresAt"));

    // Dialog may have been torn down during the nested event loop in revokeToken().
    if (!m_remoteSettings) {
        return;
    }
    // Persist cleared config
    m_remoteSettings->setProviderConfig(providerType(), ConfigName, m_config);
    m_remoteSettings->saveSettings();

    // Restore idle state
    updateAuthStatus(AuthState::Idle);
    setFieldsEnabled(true);

    emit showMessage(tr("Token revoked."), MessageWidget::Positive, false);
}

void DropboxCloudSyncPage::onTestConnectionClicked()
{
    // Validate prerequisites
    if (m_config[QStringLiteral("accessToken")].toString().isEmpty()) {
        emit showMessage(tr("Authorize first before testing the connection."), MessageWidget::Warning, false);
        return;
    }
    if (m_ui->remotePathEdit->text().trimmed().isEmpty()) {
        emit showMessage(tr("Remote path is required."), MessageWidget::Warning, false);
        return;
    }

    if (!m_dropboxProvider) {
        return;
    }

    auto params = buildDropboxParams();

    // Guard against widget destruction during nested event loops below.
    QPointer<DropboxCloudSyncPage> guard(this);

    // Refresh auth first (no-op if access token still valid). Mirrors
    // SyncEngine::doAuthenticate so a stale access token doesn't fail the test
    // when the refresh token is still valid.
    auto refreshResult = m_dropboxProvider->refreshAuth(params.get());
    if (!guard) {
        return;
    }
    if (!refreshResult.success) {
        emit showMessage(refreshResult.errorMessage, MessageWidget::Error, false);
        return;
    }
    if (!refreshResult.stdOutput.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(refreshResult.stdOutput.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject tokenData = doc.object();
            if (tokenData.contains(QStringLiteral("accessToken"))) {
                params->accessToken = tokenData[QStringLiteral("accessToken")].toString();
                m_config[QStringLiteral("accessToken")] = params->accessToken;
            }
            if (tokenData.contains(QStringLiteral("expiresAt"))) {
                params->expiresAt =
                    QDateTime::fromMSecsSinceEpoch(tokenData[QStringLiteral("expiresAt")].toVariant().toLongLong());
                m_config[QStringLiteral("expiresAt")] = tokenData[QStringLiteral("expiresAt")];
            }
            // Dialog may have been torn down during the nested event loop in refreshAuth().
            if (!m_remoteSettings) {
                return;
            }
            m_remoteSettings->setProviderConfig(providerType(), ConfigName, m_config);
        }
    }

    // Attempt download (blocks via internal QEventLoop).
    // m_dropboxProvider may have been auto-nulled during refreshAuth()'s nested event loop.
    if (!m_dropboxProvider) {
        return;
    }
    RemoteHandler::RemoteResult result = m_dropboxProvider->download(params.get());
    if (!guard) {
        return;
    }

    if (result.success) {
        if (!result.filePath.isEmpty()) {
            // Clean up temp file immediately
            QFile::remove(result.filePath);
            emit showMessage(tr("Connected. Remote file found."), MessageWidget::Positive, false);
        } else {
            emit showMessage(
                tr("Connected. File not found -- it will be created on first sync."), MessageWidget::Positive, false);
        }
    } else {
        // Prefer the kind set by the provider at the source. Falling back to
        // classifyError(errorMsg) parses raw OAuth strings (which work) plus
        // tr()'d wrapper strings (which silently miss on localized builds).
        QString errorMsg = result.errorMessage;
        auto kind = result.kind;
        // Dialog may have been torn down during the nested event loop in download().
        if (kind == RemoteSyncProvider::ErrorKind::Other && m_dropboxProvider) {
            kind = m_dropboxProvider->classifyError(errorMsg);
        }
        // Wording shared verbatim with NextcloudCloudSyncPage::onTestConnectionClicked
        // so users see the same banner for the same condition across providers.
        if (kind == RemoteSyncProvider::ErrorKind::AuthExpired || kind == RemoteSyncProvider::ErrorKind::AuthRevoked) {
            errorMsg = tr("Authorization expired. Re-authorize.");
        } else if (kind == RemoteSyncProvider::ErrorKind::Network) {
            errorMsg = tr("Network error: timeout.");
        } else if (kind == RemoteSyncProvider::ErrorKind::NotFound) {
            errorMsg = tr("Remote path not found.");
        }
        emit showMessage(errorMsg, MessageWidget::Error, false);
    }

    emit requestTestConnection();
}

void DropboxCloudSyncPage::onRemoveClicked()
{
    // If authorized, revoke tokens first (best-effort)
    if (m_authState == AuthState::Authorized) {
        if (!m_dropboxProvider) {
            return;
        }
        auto params = buildDropboxParams();
        QPointer<DropboxCloudSyncPage> guard(this);
        m_dropboxProvider->revokeToken(params.get());
        if (!guard) {
            return;
        }
    }

    // Clear cached config entirely
    m_config = QJsonObject();

    // Dialog may have been torn down during the nested event loop in revokeToken().
    if (!m_remoteSettings) {
        return;
    }
    // Remove from RemoteSettings persistence
    m_remoteSettings->removeProviderConfig(providerType(), ConfigName);
    m_remoteSettings->saveSettings();

    // Clear UI fields under QSignalBlockers -- otherwise each clear() fires
    // textChanged -> markModified -> emit modified(), which would leave Apply
    // enabled-with-nothing-to-apply (saveToConfig would just return empty).
    // Mirrors NextcloudCloudSyncPage::onRemoveClicked.
    {
        const QSignalBlocker appKeyBlocker(m_ui->appKeyEdit);
        const QSignalBlocker remotePathBlocker(m_ui->remotePathEdit);
        m_ui->appKeyEdit->clear();
        m_ui->remotePathEdit->clear();
    }

    // Restore idle state
    updateAuthStatus(AuthState::Idle);
    setFieldsEnabled(true);

    emit showMessage(tr("Cloud sync configuration removed."), MessageWidget::Positive, false);

    // After Remove, the page state matches the now-removed RemoteSettings entry;
    // Apply should disable. saveSettings() above already persisted the removal.
    m_modified = false;

    emit requestRemove();
}

void DropboxCloudSyncPage::onSubmitManualCode()
{
    const QString authCode = m_ui->manualCodeEdit->text().trimmed();
    if (authCode.isEmpty()) {
        emit showMessage(tr("Please enter the authorization code."), MessageWidget::Warning, false);
        return;
    }
    if (!m_loginFlow) {
        // Defensive: should not happen -- the manual code widget only appears
        // after onAuthorizationManualFallback fired, which means the flow exists.
        emit showMessage(tr("Authorization is not in progress. Click Authorize first."),
                         MessageWidget::Warning,
                         false);
        return;
    }

    // Hand off to the flow; outcome arrives via onAuthorizationCompleted or
    // onAuthorizationFailed. UI cleanup happens in those slots so the manual
    // code widget stays visible while the exchange POST is in flight.
    m_loginFlow->submitManualCode(authCode, 30000);
}

void DropboxCloudSyncPage::onCancelManualCode()
{
    // Cancel the flow -- onAuthorizationCancelled will reset UI state.
    if (m_loginFlow) {
        m_loginFlow->cancel();
    } else {
        // Defensive same-shape teardown if somehow the flow is null.
        m_ui->manualCodeEdit->clear();
        m_ui->manualCodeWidget->setHidden(true);
        m_ui->authorizeButton->setHidden(false);
        updateAuthStatus(AuthState::Idle);
        setFieldsEnabled(true);
    }
}

void DropboxCloudSyncPage::onTriggerSyncClicked()
{
    emit requestSync();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void DropboxCloudSyncPage::updateAuthStatus(AuthState state)
{
    m_authState = state;

    StateColorPalette statePalette;
    const QString boldGreen =
        QStringLiteral("font-weight: bold; color: %1;").arg(statePalette.color(StateColorPalette::True).name());
    const QString boldRed =
        QStringLiteral("font-weight: bold; color: %1;").arg(statePalette.color(StateColorPalette::False).name());

    switch (state) {
    case AuthState::Idle:
        m_ui->authStatusLabel->setText(tr("Not authorized"));
        m_ui->authStatusLabel->setStyleSheet(boldRed);
        m_ui->authorizeButton->setText(tr("Authorize"));
        m_ui->manualCodeWidget->setHidden(true);
        m_ui->triggerSyncButton->setEnabled(false);
        break;

    case AuthState::Authorizing:
        m_ui->authStatusLabel->setText(tr("Waiting for browser authorization..."));
        m_ui->authStatusLabel->setStyleSheet(QString());
        m_ui->authorizeButton->setText(tr("Cancel Authorization"));
        m_ui->manualCodeWidget->setHidden(true);
        m_ui->triggerSyncButton->setEnabled(false);
        break;

    case AuthState::ManualFallback:
        m_ui->authStatusLabel->setText(tr("Enter authorization code from browser"));
        m_ui->authStatusLabel->setStyleSheet(QString());
        m_ui->authorizeButton->setText(tr("Authorize"));
        m_ui->manualCodeWidget->setHidden(false);
        m_ui->triggerSyncButton->setEnabled(false);
        break;

    case AuthState::Authorized:
        m_ui->authStatusLabel->setText(tr("Authorized"));
        m_ui->authStatusLabel->setStyleSheet(boldGreen);
        m_ui->authorizeButton->setText(tr("Revoke"));
        m_ui->manualCodeWidget->setHidden(true);
        // Sync Now stays disabled while the page has unsaved edits or a
        // just-completed auth that hasn't been Applied -- syncing reads
        // persisted RemoteSettings, not page state.
        m_ui->triggerSyncButton->setEnabled(!m_modified);
        break;

    case AuthState::Revoking:
        m_ui->authStatusLabel->setText(tr("Revoking..."));
        m_ui->authStatusLabel->setStyleSheet(QString());
        m_ui->triggerSyncButton->setEnabled(false);
        break;
    }
}

void DropboxCloudSyncPage::mergeAndPersistTokens(const QString& accessToken,
                                                 const QString& refreshToken,
                                                 qint64 expiresAtMs)
{
    // Ensure config always has required metadata (auth may fire before saveSettings).
    m_config[QStringLiteral("type")] = providerType();
    m_config[QStringLiteral("name")] = ConfigName;
    m_config[QStringLiteral("appKey")] = m_ui->appKeyEdit->text().trimmed();
    m_config[QStringLiteral("remotePath")] = m_ui->remotePathEdit->text().trimmed();

    m_config[QStringLiteral("accessToken")] = accessToken;
    m_config[QStringLiteral("refreshToken")] = refreshToken;
    m_config[QStringLiteral("expiresAt")] = static_cast<double>(expiresAtMs);

    if (m_remoteSettings) {
        m_remoteSettings->setProviderConfig(providerType(), ConfigName, m_config);
    }

    // Mark the page dirty so the dialog's Apply button enables -- otherwise a
    // user who clicked Authorize without first editing the app key (key was
    // already loaded from CustomData) sees "Authorized" with Apply disabled
    // and loses tokens on dialog close.
    m_modified = true;
    emit modified();

    // Don't save the database here -- the settings dialog Apply handles
    // persistence. Saving here would trigger databaseSaved -> sync-on-save
    // while the dialog is still open, causing a duplicate sync.
}

void DropboxCloudSyncPage::ensureLoginFlow()
{
    if (m_loginFlow) {
        return;
    }
    m_loginFlow.reset(new DropboxLoginFlow(this));
    connect(m_loginFlow.data(),
            &DropboxLoginFlow::authorizationManualFallback,
            this,
            &DropboxCloudSyncPage::onAuthorizationManualFallback);
    connect(m_loginFlow.data(),
            &DropboxLoginFlow::authorizationCompleted,
            this,
            &DropboxCloudSyncPage::onAuthorizationCompleted);
    connect(m_loginFlow.data(),
            &DropboxLoginFlow::authorizationFailed,
            this,
            &DropboxCloudSyncPage::onAuthorizationFailed);
    connect(m_loginFlow.data(),
            &DropboxLoginFlow::authorizationCancelled,
            this,
            &DropboxCloudSyncPage::onAuthorizationCancelled);
}

void DropboxCloudSyncPage::setLoginFlowForTest(DropboxLoginFlow* flow)
{
    // Test seam: caller hands off ownership; we reparent and wire signals so
    // the page treats it like the lazily-constructed default.
    m_loginFlow.reset(flow);
    if (flow) {
        flow->setParent(this);
        connect(flow,
                &DropboxLoginFlow::authorizationManualFallback,
                this,
                &DropboxCloudSyncPage::onAuthorizationManualFallback);
        connect(flow,
                &DropboxLoginFlow::authorizationCompleted,
                this,
                &DropboxCloudSyncPage::onAuthorizationCompleted);
        connect(flow,
                &DropboxLoginFlow::authorizationFailed,
                this,
                &DropboxCloudSyncPage::onAuthorizationFailed);
        connect(flow,
                &DropboxLoginFlow::authorizationCancelled,
                this,
                &DropboxCloudSyncPage::onAuthorizationCancelled);
    }
}

// ---------------------------------------------------------------------------
// DropboxLoginFlow signal-slot bodies
// ---------------------------------------------------------------------------

void DropboxCloudSyncPage::onAuthorizationManualFallback(const QString& codeVerifier)
{
    Q_UNUSED(codeVerifier); // owned inside m_loginFlow; no page-side copy needed
    updateAuthStatus(AuthState::ManualFallback);
    setFieldsEnabled(true);
    // Keep the top-level Authorize button hidden during manual fallback; the
    // sub-panel's Submit / Cancel buttons drive the flow from here.
    m_ui->authorizeButton->setHidden(true);
    m_ui->manualCodeEdit->setFocus();
}

void DropboxCloudSyncPage::onAuthorizationCompleted(const QString& accessToken,
                                                    const QString& refreshToken,
                                                    qint64 expiresAtMs)
{
    mergeAndPersistTokens(accessToken, refreshToken, expiresAtMs);
    updateAuthStatus(AuthState::Authorized);
    setFieldsEnabled(true);
    // Either path through onAuthorizationCompleted leaves the manual code widget
    // hidden -- updateAuthStatus(Authorized) already hides it, but be explicit
    // about the manual-fallback-then-success path too.
    m_ui->manualCodeEdit->clear();
    m_ui->manualCodeWidget->setHidden(true);
    m_ui->authorizeButton->setHidden(false);

    emit showMessage(tr("Authorization successful, click Apply to save."), MessageWidget::Positive, false);
}

void DropboxCloudSyncPage::onAuthorizationFailed(const QString& reason)
{
    emit showMessage(reason, MessageWidget::Error, false);
    updateAuthStatus(AuthState::Idle);
    setFieldsEnabled(true);
    m_ui->manualCodeEdit->clear();
    m_ui->manualCodeWidget->setHidden(true);
    m_ui->authorizeButton->setHidden(false);
}

void DropboxCloudSyncPage::onAuthorizationCancelled()
{
    // Cancellation is silent (the user clicked Cancel themselves; surfacing a
    // banner would be redundant). Mirrors NextcloudCloudSyncPage::onLoginCancelled.
    updateAuthStatus(AuthState::Idle);
    setFieldsEnabled(true);
    m_ui->manualCodeEdit->clear();
    m_ui->manualCodeWidget->setHidden(true);
    m_ui->authorizeButton->setHidden(false);
}
