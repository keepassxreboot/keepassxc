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

#include "DropboxLoginFlow.h"

#include "OAuthHttpServer.h"

#include "core/Clock.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrlQuery>

// ---------------------------------------------------------------------------
// Localhost callback port. Dropbox PKCE flow's redirect_uri in the registered
// app is "http://localhost:12345"; the OAuthHttpServer listens on this fixed
// port. If the port is in use, the flow falls back to manual paste.
// ---------------------------------------------------------------------------
static constexpr quint16 kLocalCallbackPort = 12345;

DropboxLoginFlow::DropboxLoginFlow(QObject* parent)
    : QObject(parent)
{
    setObjectName(QStringLiteral("DropboxLoginFlow"));

    // Default browser opener: production calls QDesktopServices::openUrl.
    // Tests inject a recorder via setBrowserOpener() before driving the flow.
    m_browserOpener = [](const QUrl& url) { QDesktopServices::openUrl(url); };

    // Timer exists from construction so the dtor cleans it up unconditionally.
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &DropboxLoginFlow::onAuthTimeoutFired);
}

DropboxLoginFlow::~DropboxLoginFlow()
{
    // Cancel in-flight flow before teardown.
    cancel();
}

void DropboxLoginFlow::setNetworkAccessManager(QNetworkAccessManager* nam)
{
    if (m_nam && m_nam != nam) {
        delete m_nam;
    }
    m_nam = nam;
    if (m_nam && m_nam->parent() != this) {
        m_nam->setParent(this);
    }
}

void DropboxLoginFlow::ensureNam()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

void DropboxLoginFlow::setBrowserOpener(std::function<void(const QUrl&)> opener)
{
    if (opener) {
        m_browserOpener = std::move(opener);
    }
}

// ---------------------------------------------------------------------------
// PKCE helpers (RFC 7636).
// ---------------------------------------------------------------------------

QString DropboxLoginFlow::generateCodeVerifier()
{
    // 32 random bytes -> base64url -> ~43 chars (within RFC 7636's 43-128 range)
    QByteArray randomBytes = randomGen()->randomArray(32);
    return QString::fromLatin1(randomBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString DropboxLoginFlow::deriveCodeChallenge(const QString& codeVerifier)
{
    QByteArray hash = CryptoHash::hash(codeVerifier.toUtf8(), CryptoHash::Sha256);
    return QString::fromLatin1(hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// ---------------------------------------------------------------------------
// startAuthorization -- PKCE + browser handshake + localhost-callback / manual
// fallback decision. Fully signal-driven so the caller (DropboxCloudSyncPage)
// does not need a QPointer<self> reentrancy guard.
// ---------------------------------------------------------------------------
void DropboxLoginFlow::startAuthorization(const QString& appKey, int timeoutMs)
{
    // Cancel-previous semantics mirror NextcloudLoginFlow::startLoginFlow.
    if (m_state != State::Idle && m_state != State::Completed && m_state != State::Failed
        && m_state != State::Cancelled) {
        cancel();
    }

    if (appKey.isEmpty()) {
        // Empty appKey is a caller wiring bug -- but emit a banner rather than
        // assert because the page-side click handler should have already
        // validated. Belt-and-suspenders.
        emitFailureWithBanner(tr("App Key is required for authorization."));
        return;
    }

    m_appKey = appKey;
    m_timeoutMs = (timeoutMs > 0) ? timeoutMs : AuthTimeoutMs;

    // Generate PKCE code_verifier + code_challenge.
    m_codeVerifier = generateCodeVerifier();
    const QString codeChallenge = deriveCodeChallenge(m_codeVerifier);

    // CSRF state parameter (RFC 6749 §10.12). Tied to the localhost server's
    // expected-state validation below; only meaningful in the browser-callback
    // branch.
    QByteArray stateBytes = randomGen()->randomArray(16);
    const QString oauthState =
        QString::fromLatin1(stateBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    // Try to bind the localhost callback server. On failure, take the manual-
    // paste fallback path -- the user copies the auth code from the browser
    // and submits it via submitManualCode.
    m_server = new OAuthHttpServer(this);
    const bool serverUp = m_server->start(kLocalCallbackPort);
    bool useCopyPaste = !serverUp;

    if (serverUp) {
        m_server->setExpectedState(oauthState);
        m_redirectUri = QStringLiteral("http://localhost:%1").arg(m_server->port());
        connect(m_server, &OAuthHttpServer::authCodeReceived, this, &DropboxLoginFlow::onServerAuthCode);
        connect(m_server, &OAuthHttpServer::authError, this, &DropboxLoginFlow::onServerAuthError);
    } else {
        m_redirectUri.clear();
        delete m_server;
        m_server = nullptr;
    }

    // Build authorization URL.
    QUrl authUrl(QStringLiteral("https://www.dropbox.com/oauth2/authorize"));
    QUrlQuery authQuery;
    authQuery.addQueryItem(QStringLiteral("client_id"), m_appKey);
    authQuery.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    authQuery.addQueryItem(QStringLiteral("code_challenge"), codeChallenge);
    authQuery.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    authQuery.addQueryItem(QStringLiteral("token_access_type"), QStringLiteral("offline"));
    authQuery.addQueryItem(QStringLiteral("state"), oauthState);
    if (!useCopyPaste) {
        authQuery.addQueryItem(QStringLiteral("redirect_uri"), m_redirectUri);
    }
    authUrl.setQuery(authQuery);

    // Open the browser. No qDebug printing of the URL -- no-secrets-in-logs.
    m_browserOpener(authUrl);

    if (useCopyPaste) {
        // Manual fallback: caller collects an auth code from the user and
        // submits it via submitManualCode. The codeVerifier is part of the
        // signal payload because the caller round-trips it back into
        // submitManualCode -- this keeps DropboxLoginFlow stateful only across
        // its own lifetime, not across page-side button clicks.
        m_state = State::ManualFallback;
        emit authorizationManualFallback(m_codeVerifier);
        return;
    }

    // Browser-callback branch: arm the auth timeout and wait for the server
    // to emit authCodeReceived / authError. State stays Authorizing until one
    // of those fires (or the timer expires, or cancel() fires).
    m_state = State::Authorizing;
    m_timeoutTimer->start(m_timeoutMs);
}

// ---------------------------------------------------------------------------
// submitManualCode -- caller's response to authorizationManualFallback.
// ---------------------------------------------------------------------------
void DropboxLoginFlow::submitManualCode(const QString& authCode, int timeoutMs)
{
    if (m_state != State::ManualFallback) {
        // Out-of-state submit is a caller wiring bug -- emit a banner rather
        // than assert. Don't transition state.
        emit authorizationFailed(tr("Authorization is not awaiting a manual code."));
        return;
    }

    const QString code = authCode.trimmed();
    if (code.isEmpty()) {
        emit authorizationFailed(tr("Authorization code is required"));
        return;
    }
    if (m_codeVerifier.isEmpty()) {
        emit authorizationFailed(tr("Code verifier is missing -- restart authorization"));
        return;
    }
    if (m_appKey.isEmpty()) {
        emit authorizationFailed(tr("App Key is required"));
        return;
    }

    const int effectiveTimeout = (timeoutMs > 0) ? timeoutMs : m_timeoutMs;
    // Manual paste path: redirectUri MUST be empty -- the authorize URL had
    // no redirect_uri parameter, and Dropbox rejects mismatched redirect_uri
    // on token exchange.
    exchangeAuthCode(code, QString(), effectiveTimeout);
}

// ---------------------------------------------------------------------------
// onServerAuthCode -- localhost callback succeeded. Kick off token exchange.
// ---------------------------------------------------------------------------
void DropboxLoginFlow::onServerAuthCode(const QString& code)
{
    if (m_state != State::Authorizing) {
        return; // late-arriving signal after cancel / timeout
    }
    // Disarm the auth timer; the exchange POST has its own implicit timeout
    // via the QNetworkAccessManager call.
    m_timeoutTimer->stop();
    if (m_server) {
        m_server->stop();
    }
    exchangeAuthCode(code, m_redirectUri, m_timeoutMs);
}

void DropboxLoginFlow::onServerAuthError(const QString& error)
{
    if (m_state != State::Authorizing) {
        return;
    }
    emitFailureWithBanner(tr("Authorization failed: %1").arg(error));
}

void DropboxLoginFlow::onAuthTimeoutFired()
{
    if (m_state != State::Authorizing) {
        return;
    }
    emitFailureWithBanner(tr("Authorization timed out. Try again."));
}

// ---------------------------------------------------------------------------
// exchangeAuthCode -- async POST to /oauth2/token. No retries: the user just
// pasted a code, so a failed exchange means the user re-authorizes.
// ---------------------------------------------------------------------------
void DropboxLoginFlow::exchangeAuthCode(const QString& authCode, const QString& redirectUri, int /*timeoutMs*/)
{
    ensureNam();

    QByteArray postBody;
    postBody.append("code=");
    postBody.append(QUrl::toPercentEncoding(authCode));
    postBody.append("&grant_type=authorization_code");
    postBody.append("&code_verifier=");
    postBody.append(QUrl::toPercentEncoding(m_codeVerifier));
    postBody.append("&client_id=");
    postBody.append(QUrl::toPercentEncoding(m_appKey));

    // Only include redirect_uri if it was used in the authorize URL
    // (Dropbox's PKCE token exchange rejects mismatched redirect_uri).
    if (!redirectUri.isEmpty()) {
        postBody.append("&redirect_uri=");
        postBody.append(QUrl::toPercentEncoding(redirectUri));
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://api.dropboxapi.com/oauth2/token")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply* reply = m_nam->post(request, postBody);
    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
    }
    connect(reply, &QNetworkReply::finished, this, &DropboxLoginFlow::onExchangeFinished);

    // Zero the POST body (contains auth code + code verifier). The QNAM has
    // already taken its own copy.
    postBody.fill('\0');

    m_state = State::Exchanging;
}

void DropboxLoginFlow::onExchangeFinished()
{
    QNetworkReply* reply = nullptr;
    {
        QMutexLocker locker(&m_replyMutex);
        reply = m_activeReply.data();
        m_activeReply.clear();
    }
    if (!reply) {
        // Already torn down (cancel fired before signal). Ignore.
        return;
    }

    if (m_state != State::Exchanging) {
        // Late-arriving signal after cancel. Drain + release the reply but do
        // not emit anything (cancel already emitted authorizationCancelled).
        reply->readAll();
        reply->deleteLater();
        return;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (httpStatus == 0 && reply->error() != QNetworkReply::NoError) {
        const QString errorMsg = tr("Token exchange failed: %1").arg(reply->errorString());
        reply->deleteLater();
        emitFailureWithBanner(errorMsg);
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument respDoc = QJsonDocument::fromJson(responseData);
    QJsonObject respObj = respDoc.object();

    if (httpStatus != 200) {
        // Do not log the response body -- it can contain sensitive OAuth
        // fields. Status + length is enough for diagnostics.
        qWarning("[DPX] exchangeAuthCode: error (status %d, body length %d)", httpStatus, responseData.length());
        const QString errorTag = respObj[QStringLiteral("error")].toString();
        const QString errorDesc = respObj[QStringLiteral("error_description")].toString();
        responseData.fill('\0');
        emitFailureWithBanner(tr("Token exchange failed: %1").arg(errorDesc.isEmpty() ? errorTag : errorDesc));
        return;
    }

    // Parse successful response
    const QString accessToken = respObj[QStringLiteral("access_token")].toString();
    const QString refreshToken = respObj[QStringLiteral("refresh_token")].toString();
    const int expiresIn = respObj[QStringLiteral("expires_in")].toInt();

    if (accessToken.isEmpty() || refreshToken.isEmpty()) {
        responseData.fill('\0');
        emitFailureWithBanner(tr("Token exchange failed: missing tokens in response"));
        return;
    }

    const QDateTime expiresAt = Clock::currentDateTimeUtc().addSecs(expiresIn);
    responseData.fill('\0');

    teardown();
    m_state = State::Completed;
    emit authorizationCompleted(accessToken, refreshToken, expiresAt.toMSecsSinceEpoch());
}

// ---------------------------------------------------------------------------
// cancel -- abort any in-flight flow. Emits authorizationCancelled exactly
// once if a non-terminal flow was stopped.
// ---------------------------------------------------------------------------
void DropboxLoginFlow::cancel()
{
    const bool wasActive = (m_state == State::Authorizing || m_state == State::ManualFallback
                            || m_state == State::Exchanging);

    // Abort active reply (under mutex; marshal to network thread).
    {
        QMutexLocker locker(&m_replyMutex);
        if (m_activeReply) {
            QMetaObject::invokeMethod(m_activeReply.data(), "abort", Qt::QueuedConnection);
        }
    }

    teardown();

    if (wasActive) {
        m_state = State::Cancelled;
        emit authorizationCancelled();
    }
}

void DropboxLoginFlow::teardown()
{
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    if (m_server) {
        m_server->stop();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

void DropboxLoginFlow::emitFailureWithBanner(const QString& bannerText)
{
    teardown();
    m_state = State::Failed;
    emit authorizationFailed(bannerText);
}
