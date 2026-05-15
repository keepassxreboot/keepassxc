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

#include "NextcloudCloudSyncPage.h"
#include "ui_NextcloudCloudSyncPage.h"

#include "gui/MessageWidget.h"
#include "gui/remote/RemoteSettings.h"
#include "gui/styles/StateColorPalette.h"
#include "remotesync/NextcloudLoginFlow.h"
#include "remotesync/NextcloudSyncProvider.h"
#include "remotesync/RemoteSyncParams.h"

#include <QDesktopServices>
#include <QPointer>
#include <QSignalBlocker>
#include <QUrl>

#include <memory>

const QString NextcloudCloudSyncPage::ConfigName = QStringLiteral("nextcloud-default");

NextcloudCloudSyncPage::NextcloudCloudSyncPage(QWidget* parent)
    : CloudSyncPage(parent)
    , m_ui(new Ui::NextcloudCloudSyncPage())
{
    m_ui->setupUi(this);
    // objectName "nextcloudPage" is assigned externally by
    // CloudSyncPage::createBuiltinPages -- same pattern as DropboxCloudSyncPage.
    // The factory is the single owner of the page-level objectName so direct
    // construction in tests stays anonymous.

    // Click handlers ----------------------------------------------------
    connect(m_ui->authorizeButton, &QPushButton::clicked, this, &NextcloudCloudSyncPage::onAuthorizeClicked);
    connect(m_ui->testConnectionButton, &QPushButton::clicked, this, &NextcloudCloudSyncPage::onTestConnectionClicked);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &NextcloudCloudSyncPage::onRemoveClicked);
    connect(m_ui->triggerSyncButton, &QPushButton::clicked, this, &NextcloudCloudSyncPage::onTriggerSyncClicked);
    // appPasswordGroupBox sub-panel buttons.
    connect(m_ui->appPasswordAuthorizeButton,
            &QPushButton::clicked,
            this,
            &NextcloudCloudSyncPage::onAppPasswordAuthorizeClicked);
    connect(m_ui->openSecurityButton, &QPushButton::clicked, this, &NextcloudCloudSyncPage::onOpenSecurityClicked);

    // Modified signal wiring -------------------------------------------
    // Disabling triggerSyncButton when m_modified flips true ensures
    // Sync Now obeys the "click Apply first" banners -- syncing reads
    // from RemoteSettings (persisted), not page state, so an unsaved
    // edit would either no-op or fire stale credentials.
    auto markModified = [this] {
        m_modified = true;
        m_ui->triggerSyncButton->setEnabled(false);
        emit modified();
    };
    connect(m_ui->serverBaseUrlEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->remotePathEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->syncOnSaveCheckBox, &QCheckBox::toggled, this, markModified);
    connect(m_ui->syncOnOpenCheckBox, &QCheckBox::toggled, this, markModified);
    // 3 editable widgets in the appPasswordGroupBox sub-panel. The QGroupBox itself
    // emits toggled() when the user expands/collapses the sub-panel -- that's also a
    // modification to the page state from a user-edit perspective. loadFromConfig blocks
    // all 3 of these signals so auto-expand on dialog open does not falsely flip
    // m_modified.
    connect(m_ui->loginNameEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->appPasswordEdit, &QLineEdit::textChanged, this, markModified);
    connect(m_ui->appPasswordGroupBox, &QGroupBox::toggled, this, [this](bool) {
        m_modified = true;
        m_ui->triggerSyncButton->setEnabled(false);
        emit modified();
    });

    // Default browser opener -- production path. Tests override via
    // setBrowserOpener. Mirrors NextcloudLoginFlow's default-opener shape.
    m_browserOpener = [](const QUrl& url) { QDesktopServices::openUrl(url); };

    // No ctor-time updateAuthStatus(Idle) -- DatabaseSettingsWidgetCloudSync
    // calls loadFromConfig before showing the page, which drives the auth
    // status row from persisted state. Matches DropboxCloudSyncPage.
}

NextcloudCloudSyncPage::~NextcloudCloudSyncPage() = default;

// ---------------------------------------------------------------------------
// CloudSyncPage contract overrides
// ---------------------------------------------------------------------------

void NextcloudCloudSyncPage::setProvider(RemoteSyncProvider* provider)
{
    // Borrowed pointer; cast to concrete type for Nextcloud-specific methods
    // (Login Flow v2 / app-password orchestration -- not on the abstract base
    // since they're protocol-specific). qobject_cast inside a Nextcloud*-named
    // file is the legitimate exemption (mirrors DropboxCloudSyncPage::setProvider).
    // The provider's lifetime is owned by the widget that constructed it via
    // RemoteSyncProvider::create.
    m_nextcloudProvider = qobject_cast<NextcloudSyncProvider*>(provider);
}

QString NextcloudCloudSyncPage::providerType() const
{
    return QStringLiteral("nextcloud");
}

QString NextcloudCloudSyncPage::providerDisplayName() const
{
    // Untranslated brand identifier, matching DropboxCloudSyncPage and
    // RemoteSyncProvider::displayName's "UI applies tr() at the call site"
    // contract. The dropdown population code is the single tr() site.
    return QStringLiteral("Nextcloud");
}

void NextcloudCloudSyncPage::loadFromConfig(const QJsonObject& config)
{
    m_config = config;

    // Block signals while populating UI from config to avoid false
    // modified-flag triggers (mirrors DropboxCloudSyncPage::loadFromConfig
    // precedent).
    const QSignalBlocker serverUrlBlocker(m_ui->serverBaseUrlEdit);
    const QSignalBlocker remotePathBlocker(m_ui->remotePathEdit);
    const QSignalBlocker syncOnSaveBlocker(m_ui->syncOnSaveCheckBox);
    const QSignalBlocker syncOnOpenBlocker(m_ui->syncOnOpenCheckBox);
    // The appPasswordGroupBox blocker is load-bearing -- if the user had
    // checked the box in a prior session (paste path) and we now collapse
    // it, the unblocked setChecked(false) would fire QGroupBox::toggled
    // -> markModified, falsely enabling the parent's Apply button on a
    // mere dialog reopen.
    const QSignalBlocker loginNameBlocker(m_ui->loginNameEdit);
    const QSignalBlocker appPasswordBlocker(m_ui->appPasswordEdit);
    const QSignalBlocker appPasswordGroupBoxBlocker(m_ui->appPasswordGroupBox);

    m_ui->serverBaseUrlEdit->setText(m_config[QStringLiteral("serverBaseUrl")].toString());
    m_ui->remotePathEdit->setText(m_config[QStringLiteral("remotePath")].toString());
    m_ui->syncOnSaveCheckBox->setChecked(m_config.value(QStringLiteral("syncOnSave")).toBool(true));
    m_ui->syncOnOpenCheckBox->setChecked(m_config.value(QStringLiteral("syncOnOpen")).toBool(true));

    // Populate sub-panel fields from m_config but always leave the QGroupBox
    // collapsed. The checkbox represents the user's explicit choice to use
    // the paste-creds path INSTEAD of the top-level Authorize button (Login
    // Flow v2) -- it is NOT derived from whether creds happen to be
    // persisted. After a successful Login Flow v2 authorization (or a paste
    // saved in a prior session), loginNameEdit / appPasswordEdit are
    // populated but stay visible-but-grayed (QGroupBox checkable+unchecked
    // -> children disabled), so the user sees the saved creds without being
    // able to edit them by accident. Clicking the box is the user's
    // affirmative "I want to edit/paste" action.
    const QString loginName = m_config[QStringLiteral("loginName")].toString();
    const QString appPassword = m_config[QStringLiteral("appPassword")].toString();
    m_ui->loginNameEdit->setText(loginName);
    m_ui->appPasswordEdit->setText(appPassword);
    m_ui->appPasswordGroupBox->setChecked(false);

    // Drive the auth-status row from persisted creds. If both fields are
    // populated, the page reopens directly into the Authorized state
    // ("Authorized as <loginName>" + button text "Remove"). The
    // loginCompleted slot writes those keys after a successful Login Flow v2;
    // this branch handles the dialog-reopen path.
    if (!loginName.isEmpty() && !appPassword.isEmpty()) {
        m_authState = AuthState::Authorized;
    } else {
        m_authState = AuthState::Idle;
    }
    // Reset m_modified BEFORE updateAuthStatus so the Authorized branch's
    // triggerSyncButton gate sees the post-load state (clean, not dirty).
    m_modified = false;
    updateAuthStatus(m_authState);
}

QJsonObject NextcloudCloudSyncPage::saveToConfig() const
{
    // Fresh-no-edit fast path (mirrors DropboxCloudSyncPage::saveToConfig):
    // every editable field empty AND no cached config means the user opened
    // the dialog, selected Nextcloud in the combo, and never touched
    // anything. Return an empty object so the parent skips persistence --
    // otherwise the always-populated type/name keys below would mark
    // Nextcloud as the active provider with no credentials, leaving cloud
    // sync "configured but unusable."
    if (m_ui->serverBaseUrlEdit->text().trimmed().isEmpty()
        && m_ui->remotePathEdit->text().trimmed().isEmpty()
        && m_ui->loginNameEdit->text().trimmed().isEmpty()
        && m_ui->appPasswordEdit->text().isEmpty()
        && m_config.isEmpty()) {
        return QJsonObject();
    }

    // Merge over m_config (rather than replacing) to preserve any keys set by
    // the authorization paths (loginName / appPassword from onLoginCompleted
    // or onAppPasswordAuthorizeClicked) so successive loadFromConfig sees the
    // persisted creds.
    QJsonObject config = m_config;
    config[QStringLiteral("type")] = providerType();
    config[QStringLiteral("name")] = ConfigName;
    config[QStringLiteral("serverBaseUrl")] = m_ui->serverBaseUrlEdit->text().trimmed();
    config[QStringLiteral("remotePath")] = NextcloudSyncProvider::normalizeRemotePath(m_ui->remotePathEdit->text());
    config[QStringLiteral("syncOnSave")] = m_ui->syncOnSaveCheckBox->isChecked();
    config[QStringLiteral("syncOnOpen")] = m_ui->syncOnOpenCheckBox->isChecked();

    // App-password sub-panel fields are the "paste-without-clicking-Authorize-
    // with-AppPassword" path. Only let them override m_config when they
    // actually contain something -- otherwise an empty sub-panel (Login
    // Flow v2 user who never touched the sub-panel) would wipe the creds
    // onLoginCompleted just persisted to m_config. App-password is NOT
    // trimmed because whitespace might be significant for app-password
    // values.
    const QString uiLoginName = m_ui->loginNameEdit->text().trimmed();
    const QString uiAppPassword = m_ui->appPasswordEdit->text();
    if (!uiLoginName.isEmpty()) {
        config[QStringLiteral("loginName")] = uiLoginName;
    }
    if (!uiAppPassword.isEmpty()) {
        config[QStringLiteral("appPassword")] = uiAppPassword;
    }

    return config;
}

bool NextcloudCloudSyncPage::isModified() const
{
    return m_modified;
}

void NextcloudCloudSyncPage::setRemoteSettings(RemoteSettings* settings)
{
    m_remoteSettings = settings;
}

void NextcloudCloudSyncPage::setBrowserOpener(BrowserOpener opener)
{
    // Test seam -- mirrors NextcloudLoginFlow::setBrowserOpener. Empty
    // std::function would silently no-op the security deep-link; ignore
    // that case so production never accidentally clears the default lambda.
    if (opener) {
        m_browserOpener = std::move(opener);
    }
}

void NextcloudCloudSyncPage::setLoginFlowForTest(NextcloudLoginFlow* flow)
{
    // Test seam: caller hands off ownership; we reparent and wire signals so
    // the page treats it like the lazily-constructed default. Mirror of
    // DropboxCloudSyncPage::setLoginFlowForTest.
    m_loginFlow.reset(flow);
    if (flow) {
        flow->setParent(this);
        connect(flow, &NextcloudLoginFlow::loginInitiated, this, &NextcloudCloudSyncPage::onLoginInitiated);
        connect(flow, &NextcloudLoginFlow::loginCompleted, this, &NextcloudCloudSyncPage::onLoginCompleted);
        connect(flow, &NextcloudLoginFlow::loginFailed, this, &NextcloudCloudSyncPage::onLoginFailed);
        connect(flow, &NextcloudLoginFlow::loginCancelled, this, &NextcloudCloudSyncPage::onLoginCancelled);
    }
}

void NextcloudCloudSyncPage::setMutualExclusivityWarning(bool active)
{
    // Disable fields under the warning so the user cannot edit Nextcloud
    // config while Script Sync is also configured (mirrors Dropbox).
    m_mutualExclusivityActive = active;
    setFieldsEnabled(!active);
}

void NextcloudCloudSyncPage::setFieldsEnabled(bool enabled)
{
    m_ui->serverBaseUrlEdit->setEnabled(enabled);
    m_ui->remotePathEdit->setEnabled(enabled);
    m_ui->authorizeButton->setEnabled(enabled);
    m_ui->testConnectionButton->setEnabled(enabled);
    m_ui->removeButton->setEnabled(enabled);
    m_ui->syncOnSaveCheckBox->setEnabled(enabled);
    m_ui->syncOnOpenCheckBox->setEnabled(enabled);
    m_ui->appPasswordGroupBox->setEnabled(enabled);
}

// ---------------------------------------------------------------------------
// Click handlers
// ---------------------------------------------------------------------------

void NextcloudCloudSyncPage::onAuthorizeClicked()
{
    // The top-level Authorize button is overloaded by AuthState:
    //   Idle        -> begin Login Flow v2
    //   Authorizing -> cancel the in-flight flow
    //   Authorized  -> route to Remove handler (button text reads "Remove" in
    //                  Authorized state; Login Flow v2 has no server-side
    //                  revoke endpoint, so "Remove" clears local config only)
    if (m_authState == AuthState::Authorized) {
        onRemoveClicked();
        return;
    }

    if (m_authState == AuthState::Authorizing) {
        // NextcloudLoginFlow::cancel is idempotent on Idle and emits
        // loginCancelled exactly once when it stops an active flow. The
        // loginCancelled signal will reset our AuthState via
        // onLoginCancelled.
        m_loginFlow->cancel();
        return;
    }

    // Idle: validate input, then start Login Flow v2. Single checkpoint via
    // the shared helper -- Empty / NotSecure / Malformed all dispatched there
    // with their own banner; we proceed only on Ok.
    const QString serverUrl = m_ui->serverBaseUrlEdit->text().trimmed();
    QString canonical;
    if (!validateAndCanonicalizeServerUrl(serverUrl, canonical)) {
        return;
    }

    // Lazy-construct NextcloudLoginFlow on first Idle Authorize click; reuse
    // the same instance on subsequent clicks. QScopedPointer gives
    // deterministic destruction; startLoginFlow is internally
    // cancel-previous, so reuse has equivalent semantics with less destructor
    // churn.
    if (!m_loginFlow) {
        m_loginFlow.reset(new NextcloudLoginFlow(this));
        connect(
            m_loginFlow.data(), &NextcloudLoginFlow::loginInitiated, this, &NextcloudCloudSyncPage::onLoginInitiated);
        connect(
            m_loginFlow.data(), &NextcloudLoginFlow::loginCompleted, this, &NextcloudCloudSyncPage::onLoginCompleted);
        connect(m_loginFlow.data(), &NextcloudLoginFlow::loginFailed, this, &NextcloudCloudSyncPage::onLoginFailed);
        connect(
            m_loginFlow.data(), &NextcloudLoginFlow::loginCancelled, this, &NextcloudCloudSyncPage::onLoginCancelled);
    }

    updateAuthStatus(AuthState::Authorizing);
    setFieldsEnabled(false);
    // Keep the Authorize button live so its "Cancel Authorization" label is clickable.
    m_ui->authorizeButton->setEnabled(true);
    m_loginFlow->startLoginFlow(serverUrl);
}

void NextcloudCloudSyncPage::onAppPasswordAuthorizeClicked()
{
    const QString serverUrl = m_ui->serverBaseUrlEdit->text().trimmed();
    QString canonical;
    if (!validateAndCanonicalizeServerUrl(serverUrl, canonical)) {
        return;
    }

    const QString loginName = m_ui->loginNameEdit->text().trimmed();
    // App-password is NOT trimmed -- whitespace might be significant.
    const QString appPassword = m_ui->appPasswordEdit->text();
    if (loginName.isEmpty() || appPassword.isEmpty()) {
        emit showMessage(tr("Enter both username and app-password."), MessageWidget::Warning, false);
        return;
    }

    // Build params from the SUB-PANEL pasted creds (NOT from m_config). The
    // QGroupBox sub-panel is the explicit pasted-validation path; the user
    // typed these values right above this button and expects them to be the
    // source of truth for THIS click.
    auto params = std::make_unique<NextcloudSyncParams>();
    params->type = providerType();
    params->name = ConfigName;
    params->serverBaseUrl = canonical;
    params->remotePath = NextcloudSyncProvider::normalizeRemotePath(m_ui->remotePathEdit->text());
    params->loginName = loginName;
    params->appPassword = appPassword;
    params->timeoutMsec = 30000;

    // Reentrancy guard. NextcloudSyncProvider::testConnection is synchronous
    // via an internal QEventLoop. If the user closes the dialog while the
    // PROPFIND is in flight, the page widget is destroyed under us;
    // dereferencing `this` after the blocking call returns would segfault.
    // The QPointer tracks widget lifetime; we bail out without touching
    // `this` if it's been deleted.
    QPointer<NextcloudCloudSyncPage> guard(this);

    setFieldsEnabled(false);
    // Keep the appPasswordAuthorizeButton itself disabled too -- setFieldsEnabled disables
    // the QGroupBox, which propagates to the button, but be explicit so a future
    // setFieldsEnabled refactor doesn't accidentally leave this button live during the
    // blocking call.
    m_ui->appPasswordAuthorizeButton->setEnabled(false);

    if (!m_nextcloudProvider) {
        return;
    }
    RemoteHandler::RemoteResult result = m_nextcloudProvider->testConnection(params.get());

    if (!guard) {
        // Widget destroyed during the call -- bail without dereferencing this.
        return;
    }

    setFieldsEnabled(true);
    m_ui->appPasswordAuthorizeButton->setEnabled(true);

    if (result.success) {
        // Persist to m_config (last-write-wins, single-slot semantics). Mirror
        // serverBaseUrl + remotePath from UI too so onTestConnectionClicked
        // (which reads from m_config) sees a fully-consistent post-Authorize
        // state. Do NOT call m_remoteSettings->saveSettings() here -- the
        // user's Apply click is the persist gate
        // (DatabaseSettingsWidgetCloudSync::saveSettings). m_modified + emit
        // modified() is what actually flips the parent's Apply button enabled
        // state.
        m_config[QStringLiteral("loginName")] = loginName;
        m_config[QStringLiteral("appPassword")] = appPassword;
        m_config[QStringLiteral("serverBaseUrl")] = serverUrl;
        m_config[QStringLiteral("remotePath")] = NextcloudSyncProvider::normalizeRemotePath(m_ui->remotePathEdit->text());
        m_modified = true;
        emit modified();

        updateAuthStatus(AuthState::Authorized);

        // KEEP the QGroupBox open after success so the user can verify their
        // persisted creds. Do NOT call appPasswordGroupBox->setChecked(false).
        // User mental model: "the creds are right here in front of me, I can
        // verify them."

        emit showMessage(tr("Authorization successful, click Apply to save."), MessageWidget::Positive, false);
    } else {
        // The provider's classifyError + mapWebdavStatusToMessage chain
        // already produces the locked verbatim credential-rejection banner.
        // Forward result.errorMessage verbatim (already tr()'d at emit site).
        emit showMessage(result.errorMessage, MessageWidget::Error, false);
        // Do NOT persist on failure: validation failure surfaces a clear
        // error and credentials are NOT persisted. m_config is unchanged;
        // m_modified is also unchanged so the parent's Apply button stays in
        // whatever state the user left it.
    }
}

void NextcloudCloudSyncPage::onOpenSecurityClicked()
{
    QString canonical;
    if (!validateAndCanonicalizeServerUrl(m_ui->serverBaseUrlEdit->text().trimmed(), canonical)) {
        return;
    }

    // Use the canonical base, NOT buildResourceUrl: buildResourceUrl produces
    // /remote.php/dav/files/<loginName>/<remotePath> (the WebDAV per-user
    // files endpoint); the Security page is at the application root. The
    // canonical form has already had its trailing slash stripped by the
    // helper, so the concat below cannot produce a double-slash regardless
    // of what the user typed.
    const QUrl securityUrl(canonical + QStringLiteral("/settings/user/security"));

    // Production: m_browserOpener is QDesktopServices::openUrl; tests inject
    // a capture lambda via setBrowserOpener(). NO qDebug printing the URL --
    // no-secrets-in-logs (the Security page URL is at the app root and
    // doesn't carry a token, but the policy blanket-covers all auth-context
    // URL logging).
    m_browserOpener(securityUrl);
}

void NextcloudCloudSyncPage::onTestConnectionClicked()
{
    // Saved-creds path (mirrors Dropbox onTestConnectionClicked). The
    // sub-panel's "Authorize with App Password" button covers pasted-creds
    // validation; this top-level Test Connection button uses the persisted
    // m_config creds so the user can verify the saved-creds path works
    // without re-typing.
    const QString loginName = m_config[QStringLiteral("loginName")].toString();
    const QString appPassword = m_config[QStringLiteral("appPassword")].toString();
    const QString serverUrl = m_config[QStringLiteral("serverBaseUrl")].toString();

    if (serverUrl.isEmpty() || loginName.isEmpty() || appPassword.isEmpty()) {
        emit showMessage(tr("Authorize Nextcloud first to test the connection."), MessageWidget::Warning, false);
        return;
    }

    auto params = std::make_unique<NextcloudSyncParams>();
    params->type = providerType();
    params->name = ConfigName;
    params->serverBaseUrl = NextcloudSyncProvider::canonicalizeServerBaseUrl(serverUrl);
    params->remotePath = m_config[QStringLiteral("remotePath")].toString();
    params->loginName = loginName;
    params->appPassword = appPassword;
    params->timeoutMsec = 30000;

    // Reentrancy guard (see onAppPasswordAuthorizeClicked above; same
    // QEventLoop synchronous-call pattern).
    QPointer<NextcloudCloudSyncPage> guard(this);
    setFieldsEnabled(false);
    m_ui->testConnectionButton->setEnabled(false);

    if (!m_nextcloudProvider) {
        return;
    }
    RemoteHandler::RemoteResult result = m_nextcloudProvider->testConnection(params.get());

    if (!guard) {
        // Widget destroyed during the call -- bail without dereferencing this.
        return;
    }

    setFieldsEnabled(true);
    m_ui->testConnectionButton->setEnabled(true);

    if (result.success) {
        // Mirror Dropbox: empty filePath signals the first-sync case (404 --
        // auth OK, file not yet on server). Differentiate the message
        // accordingly.
        if (!result.filePath.isEmpty()) {
            emit showMessage(tr("Nextcloud connection successful."), MessageWidget::Positive, false);
        } else {
            emit showMessage(
                tr("Connected. File not found -- it will be created on first sync."), MessageWidget::Positive, false);
        }
        return;
    }

    // Failure: classifyError dispatch -> user-friendly banner family. Mirror
    // DropboxCloudSyncPage verbatim with Nextcloud-flavored field reads. The
    // 4 friendliest ErrorKinds get per-kind tr() text; the rest fall through
    // to the verbatim banner from mapWebdavStatusToMessage (Conflict /
    // RateLimit / Permission / Quota / ServerError / Other are preserved
    // as-is).
    QString errorMsg = result.errorMessage;
    // Prefer the kind set by the provider at the source. Falling back to
    // classifyError(errorMsg) parses tr()'d English fragments and silently
    // misses on localized builds.
    auto kind = result.kind;
    // Dialog may have been torn down during the nested event loop in testConnection().
    if (kind == RemoteSyncProvider::ErrorKind::Other && m_nextcloudProvider) {
        kind = m_nextcloudProvider->classifyError(errorMsg);
    }
    if (kind == RemoteSyncProvider::ErrorKind::AuthExpired || kind == RemoteSyncProvider::ErrorKind::AuthRevoked) {
        errorMsg = tr("Authorization expired. Re-authorize.");
    } else if (kind == RemoteSyncProvider::ErrorKind::Network) {
        errorMsg = tr("Network error: timeout.");
    } else if (kind == RemoteSyncProvider::ErrorKind::NotFound) {
        errorMsg = tr("Remote path not found.");
    }
    emit showMessage(errorMsg, MessageWidget::Error, false);
}

void NextcloudCloudSyncPage::onRemoveClicked()
{
    // No server-side revoke. App-passwords don't have a public revoke
    // endpoint; the user revokes from the Nextcloud Security page (the
    // deep-link button is in the QGroupBox sub-panel). Clear local config
    // only.
    //
    // No QMessageBox confirmation here. The user can re-Authorize in one
    // click + browser grant.

    updateAuthStatus(AuthState::Removing);

    if (!m_remoteSettings) {
        return;
    }
    m_remoteSettings->clearCloudSyncConfig();
    m_remoteSettings->saveSettings();
    m_config = QJsonObject();

    // Clear UI fields. QSignalBlockers prevent the clears from firing
    // markModified; otherwise each clear() would emit textChanged ->
    // markModified -> emit modified() -> noisy state churn.
    {
        const QSignalBlocker serverUrlBlocker(m_ui->serverBaseUrlEdit);
        const QSignalBlocker remotePathBlocker(m_ui->remotePathEdit);
        const QSignalBlocker loginNameBlocker(m_ui->loginNameEdit);
        const QSignalBlocker appPasswordBlocker(m_ui->appPasswordEdit);
        const QSignalBlocker groupBoxBlocker(m_ui->appPasswordGroupBox);
        // syncOnSaveCheckBox / syncOnOpenCheckBox INTENTIONALLY NOT CLEARED -- those
        // are user preferences, not credentials. Mirrors Dropbox onRemoveClicked.
        m_ui->serverBaseUrlEdit->clear();
        m_ui->remotePathEdit->clear();
        m_ui->loginNameEdit->clear();
        m_ui->appPasswordEdit->clear();
        m_ui->appPasswordGroupBox->setChecked(false);
    }

    updateAuthStatus(AuthState::Idle);

    // Two-sentence post-Remove banner. Justified Dropbox-deviation:
    // structural difference is no server-side revoke endpoint, so the user
    // needs to know the next step. C++ adjacent-string-literal concatenation
    // joins these two pieces into a single banner at compile time; the
    // source split is purely a clang-format ColumnLimit:120 accommodation.
    emit showMessage(tr("Nextcloud configuration removed. "
                        "To revoke the app-password server-side, visit your Nextcloud Security page."),
                     MessageWidget::Positive,
                     false);

    // After Remove, the page state matches the now-removed RemoteSettings entry. Apply
    // button should disable. (saveSettings() above already persisted the removal;
    // m_modified=false signals the parent widget that nothing further needs persisting.)
    m_modified = false;
}

void NextcloudCloudSyncPage::onTriggerSyncClicked()
{
    emit requestSync();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void NextcloudCloudSyncPage::updateAuthStatus(AuthState state)
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
        m_ui->triggerSyncButton->setEnabled(false);
        break;

    case AuthState::Authorizing:
        // Verbatim Dropbox status copy.
        m_ui->authStatusLabel->setText(tr("Waiting for browser authorization..."));
        m_ui->authStatusLabel->setStyleSheet(QString());
        m_ui->authorizeButton->setText(tr("Cancel Authorization"));
        m_ui->triggerSyncButton->setEnabled(false);
        break;

    case AuthState::Authorized:
        m_ui->authStatusLabel->setText(tr("Authorized as %1").arg(m_config[QStringLiteral("loginName")].toString()));
        m_ui->authStatusLabel->setStyleSheet(boldGreen);
        // Button text in Authorized state is "Remove" (NOT "Revoke") for
        // Nextcloud, since Login Flow v2 has no server-side revoke endpoint
        // -- the user removes the local app-password instead.
        m_ui->authorizeButton->setText(tr("Remove"));
        // Sync Now stays disabled while the page has unsaved edits or a
        // just-completed auth that hasn't been Applied -- syncing reads
        // persisted RemoteSettings, not page state.
        m_ui->triggerSyncButton->setEnabled(!m_modified);
        break;

    case AuthState::Removing:
        m_ui->authStatusLabel->setText(tr("Removing..."));
        m_ui->authStatusLabel->setStyleSheet(QString());
        m_ui->triggerSyncButton->setEnabled(false);
        break;
    }
}

// ---------------------------------------------------------------------------
// NextcloudLoginFlow signal-slot bodies
// ---------------------------------------------------------------------------

void NextcloudCloudSyncPage::onLoginInitiated(const QUrl& loginUrl)
{
    Q_UNUSED(loginUrl);
    // Intentional no-op. The status row was already set to "Waiting for
    // browser authorization..." when onAuthorizeClicked called
    // updateAuthStatus(AuthState::Authorizing). No bridge text during the
    // browser-open phase.
    //
    // No qDebug printing loginUrl -- no-secrets-in-logs (loginUrl carries
    // the polling token in its query/fragment).
}

void NextcloudCloudSyncPage::onLoginCompleted(const QString& loginName, const QString& appPassword)
{
    // Persist creds to m_config (last-write-wins, single-slot semantics).
    // Also mirror serverBaseUrl + remotePath from the UI into m_config so
    // the post-Authorize m_config is fully consistent --
    // onTestConnectionClicked reads serverBaseUrl from m_config, and would
    // otherwise warn "Authorize first" even though the page is already in
    // Authorized state.
    //
    // Do NOT call m_remoteSettings->saveSettings() here -- that would
    // trigger databaseSaved -> sync-on-save while the dialog is still open,
    // causing a duplicate sync. The user's Apply click is the persist gate
    // (DatabaseSettingsWidgetCloudSync::saveSettings).
    m_config[QStringLiteral("loginName")] = loginName;
    m_config[QStringLiteral("appPassword")] = appPassword;
    m_config[QStringLiteral("serverBaseUrl")] = m_ui->serverBaseUrlEdit->text().trimmed();
    m_config[QStringLiteral("remotePath")] = NextcloudSyncProvider::normalizeRemotePath(m_ui->remotePathEdit->text());
    m_modified = true;
    emit modified();

    // Mirror the just-received creds into the sub-panel UI fields so the user sees
    // "Authorized as <name>" AND the populated sub-panel without having to close
    // and reopen the dialog. QSignalBlockers prevent textChanged -> markModified
    // from re-emitting modified() (already emitted above with the correct intent).
    {
        const QSignalBlocker loginNameBlocker(m_ui->loginNameEdit);
        const QSignalBlocker appPasswordBlocker(m_ui->appPasswordEdit);
        m_ui->loginNameEdit->setText(loginName);
        m_ui->appPasswordEdit->setText(appPassword);
    }

    setFieldsEnabled(true);
    // updateAuthStatus(Authorized) renders "Authorized as <loginName>" and
    // flips the top button text to "Remove".
    updateAuthStatus(AuthState::Authorized);

    emit showMessage(tr("Authorization successful, click Apply to save."), MessageWidget::Positive, false);
}

void NextcloudCloudSyncPage::onLoginFailed(const QString& reason)
{
    setFieldsEnabled(true);
    updateAuthStatus(AuthState::Idle);

    // `reason` is verbatim from NextcloudLoginFlow's banner constants
    // (timeout / initiate-fail / network-error / authcheck-fail / cancelled
    // strings, already tr()'d at the emit sites). Do NOT re-tr() here -- it
    // would create a duplicate translation site for the same string.
    emit showMessage(reason, MessageWidget::Error, false);
}

void NextcloudCloudSyncPage::onLoginCancelled()
{
    setFieldsEnabled(true);
    updateAuthStatus(AuthState::Idle);
    // Cancellation is silent (the user clicked Cancel themselves; surfacing
    // a banner would be redundant).
}

bool NextcloudCloudSyncPage::validateAndCanonicalizeServerUrl(const QString& input, QString& canonicalOut)
{
    using V = NextcloudSyncProvider::ServerUrlValidity;
    QString canonical;
    const V result = NextcloudSyncProvider::validateServerUrl(input, &canonical);
    switch (result) {
    case V::Ok:
        canonicalOut = canonical;
        return true;
    case V::Empty:
        // Warning, not Error -- this is a "you forgot to type" prompt, not a
        // rejected attempt. Verbatim banner shared with the two other handlers
        // (user mental model: same prompt regardless of which button they hit).
        emit showMessage(tr("Enter the Nextcloud server URL first."), MessageWidget::Warning, false);
        return false;
    case V::NotSecure:
        // Error severity: this is an actionable rejection ("change your input
        // to https://"), not just a prompt. The banner is the only place that
        // surfaces the cleartext-policy reason; downstream layers (canonicalize
        // -> empty -> generic "URL is required") would lose this discrimination.
        emit showMessage(tr("Plain HTTP is only allowed for a loopback address (localhost / 127.0.0.1 / [::1]). "
                            "Use https:// so your Nextcloud app password is not sent in cleartext."),
                         MessageWidget::Error,
                         false);
        return false;
    case V::Malformed:
        emit showMessage(tr("Invalid Nextcloud server URL."), MessageWidget::Warning, false);
        return false;
    }
    // Unreachable: all enum cases are handled above. The return satisfies
    // compilers that don't recognize the switch as exhaustive.
    return false;
}
