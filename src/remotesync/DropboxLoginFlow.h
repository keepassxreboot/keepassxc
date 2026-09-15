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

#ifndef KEEPASSXC_DROPBOXLOGINFLOW_H
#define KEEPASSXC_DROPBOXLOGINFLOW_H

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class OAuthHttpServer;

/**
 * Dropbox OAuth2 + PKCE login flow driver: PKCE generation + browser handshake
 * + localhost callback OR manual paste fallback + authorization-code exchange.
 *
 * Self-contained auth class owning its transport and exposing terminal
 * signals, so DropboxSyncProvider stays focused on download / upload /
 * refreshAuth.
 */
class DropboxLoginFlow : public QObject
{
    Q_OBJECT

public:
    explicit DropboxLoginFlow(QObject* parent = nullptr);
    ~DropboxLoginFlow() override;

    // Begin a PKCE OAuth2 handshake: generate code_verifier + state, start the
    // localhost callback server (or fall back to manual paste), build the
    // authorize URL, open it in the user's browser, and arm the auth timeout.
    // Cancels any previously in-flight flow first.
    //
    // If the localhost server fails to start (port conflict, sandboxed
    // environment, etc.), emits authorizationManualFallback(codeVerifier)
    // synchronously and enters ManualFallback state -- the caller is expected
    // to collect an auth code from the user and submit it via submitManualCode.
    //
    // Virtual so MockDropboxLoginFlow can override for page-level UI tests
    // (avoids exercising real PKCE / browser-open / OAuthHttpServer in unit
    // tests).
    virtual void startAuthorization(const QString& appKey, int timeoutMs);

    // Exchange a manually-pasted authorization code for tokens. Used after
    // authorizationManualFallback was emitted -- redirect_uri is intentionally
    // empty per Dropbox's PKCE rule that the token exchange's redirect_uri
    // must match the authorize URL's (which, in manual fallback, had none).
    virtual void submitManualCode(const QString& authCode, int timeoutMs);

    // Cancel any in-flight flow and emit authorizationCancelled. Safe to call
    // from any thread; reply abort is marshalled to the network thread via
    // QueuedConnection. Idempotent on terminal / Idle state.
    virtual void cancel();

    // Test seam -- production lazy-constructs the default. Caller retains
    // ownership of the injected NAM.
    void setNetworkAccessManager(QNetworkAccessManager* nam);

    // Test seam -- production uses QDesktopServices::openUrl. Empty
    // std::function is ignored.
    void setBrowserOpener(std::function<void(const QUrl&)> opener);

    // Production browser-auth timeout: 2 minutes.
    static constexpr int AuthTimeoutMs = 120000;

    // PKCE helpers (public static -- pure functions, no side effects, needed
    // for testing).
    static QString generateCodeVerifier();
    static QString deriveCodeChallenge(const QString& codeVerifier);

signals:
    // Emitted exactly once when the localhost callback server fails to start
    // and the caller must collect a pasted auth code from the user. The
    // codeVerifier payload is the PKCE verifier the caller has to round-trip
    // back via submitManualCode.
    void authorizationManualFallback(QString codeVerifier);

    // Emitted on successful token exchange -- payload is the credentials the
    // caller persists. expiresAtMs is QDateTime::toMSecsSinceEpoch on the
    // computed expiry (Clock::currentDateTimeUtc + expires_in).
    void authorizationCompleted(QString accessToken, QString refreshToken, qint64 expiresAtMs);

    // Emitted on hard failure (browser-auth timeout, server-side OAuth error,
    // exchange-POST network/HTTP failure) -- carries the user-facing banner
    // verbatim.
    void authorizationFailed(QString reason);

    // Emitted exactly once when cancel() succeeds in stopping a non-terminal
    // flow.
    void authorizationCancelled();

private slots:
    // OAuthHttpServer::authCodeReceived -- kicks off the token exchange.
    void onServerAuthCode(const QString& code);

    // OAuthHttpServer::authError -- emits authorizationFailed.
    void onServerAuthError(const QString& error);

    // m_timeoutTimer fired -- emits authorizationFailed and tears down.
    void onAuthTimeoutFired();

    // QNetworkReply::finished on the token-exchange POST -- parses tokens,
    // emits authorizationCompleted or authorizationFailed.
    void onExchangeFinished();

private:
    // State machine for the login-flow lifecycle.
    enum class State
    {
        Idle,
        Authorizing, // server listening, waiting for browser callback
        ManualFallback, // server failed to start, waiting for submitManualCode
        Exchanging, // token-exchange POST in flight
        Completed,
        Failed,
        Cancelled
    };

    // Lazy-construct the QNetworkAccessManager, or use the injected one.
    void ensureNam();

    // Internal exchange entry. Builds the application/x-www-form-urlencoded
    // body, posts to /oauth2/token, wires onExchangeFinished. redirectUri is
    // empty for manual paste, set to http://localhost:<port> for the browser
    // callback path.
    void exchangeAuthCode(const QString& authCode, const QString& redirectUri, int timeoutMs);

    // Tear down server + timer + active reply under mutex. Called from
    // terminal transitions. Idempotent.
    void teardown();

    // DRY emit helper: transition to Failed and emit authorizationFailed.
    void emitFailureWithBanner(const QString& bannerText);

    QNetworkAccessManager* m_nam = nullptr;
    OAuthHttpServer* m_server = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    std::function<void(const QUrl&)> m_browserOpener;

    QString m_appKey;
    QString m_codeVerifier;
    QString m_redirectUri; // empty in ManualFallback, "http://localhost:<port>" otherwise
    int m_timeoutMs = AuthTimeoutMs;

    QPointer<QNetworkReply> m_activeReply;
    mutable QMutex m_replyMutex;

    State m_state = State::Idle;

    Q_DISABLE_COPY(DropboxLoginFlow)
};

#endif // KEEPASSXC_DROPBOXLOGINFLOW_H
