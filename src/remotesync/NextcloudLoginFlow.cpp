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

#include "NextcloudLoginFlow.h"

#include "NextcloudSyncProvider.h"
#include "config-keepassx.h"

#include <QByteArray>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

// ---------------------------------------------------------------------------
// Locked banner strings.
//   TIMEOUT:       5-minute hard timeout fired.
//   PHISHING:      phishing-mitigation -- loginUrl host/port did not match configured server.
//   INITIATE_FAIL: initiate POST failure (network error / malformed JSON / missing
//                  fields) -- same locked text in all three sub-cases.
//   NETWORK_ERROR: any polling-side hard failure (401/403/5xx/other-4xx, network error,
//                  200+missing key, 200+empty value -- server defects surface through
//                  the network-error banner).
// ---------------------------------------------------------------------------
static const char* const BANNER_TIMEOUT = "Nextcloud authorization timed out. Click Authorize to try again.";
static const char* const BANNER_PHISHING =
    "Nextcloud returned an unexpected authorization URL. Verify your server URL is correct.";
static const char* const BANNER_INITIATE_FAIL =
    "Could not start Nextcloud authorization. Verify your server URL and try again.";
static const char* const BANNER_NETWORK_ERROR =
    "Lost connection to Nextcloud during authorization. Click Authorize to try again.";

NextcloudLoginFlow::NextcloudLoginFlow(QObject* parent)
    : QObject(parent)
{
    setObjectName(QStringLiteral("NextcloudLoginFlow"));

    // Default browser opener: production calls QDesktopServices::openUrl.
    // Tests inject a recorder via setBrowserOpener() before driving startLoginFlow.
    m_browserOpener = [](const QUrl& loginUrl) { QDesktopServices::openUrl(loginUrl); };

    // Timers exist from construction so the dtor cleans them up
    // unconditionally.
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &NextcloudLoginFlow::onPollTick);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &NextcloudLoginFlow::onPollTimeoutFired);
}

NextcloudLoginFlow::~NextcloudLoginFlow()
{
    // Cancel in-flight flow before teardown.
    cancel();
}

void NextcloudLoginFlow::setNetworkAccessManager(QNetworkAccessManager* nam)
{
    // Caller-owned; see NextcloudLoginFlow.h.
    m_nam = nam;
}

void NextcloudLoginFlow::ensureNam()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

void NextcloudLoginFlow::setBrowserOpener(std::function<void(const QUrl&)> opener)
{
    if (opener) {
        m_browserOpener = std::move(opener);
    }
}

void NextcloudLoginFlow::setPollIntervalMsForTest(int ms)
{
    m_pollIntervalMs = ms;
}

void NextcloudLoginFlow::setTimeoutMsForTest(int ms)
{
    m_pollTimeoutMs = ms;
}

void NextcloudLoginFlow::startLoginFlow(const QString& serverBaseUrl)
{
    // Cancel any in-flight previous flow first. cancel() is idempotent on
    // Idle state (no-op) and tears down both timers + active reply on
    // Polling/Initiating state, emitting loginCancelled exactly once. MUST
    // be the first statement so a rapid double-Authorize never produces two
    // concurrent poll loops.
    cancel();

    m_serverBaseUrl = NextcloudSyncProvider::canonicalizeServerBaseUrl(serverBaseUrl);
    m_state = State::Initiating;

    ensureNam();

    // Compose initiate URL: <canonicalBase>/index.php/login/v2 (preserves
    // any configured subpath like "/nextcloud"). DecodedMode mirrors the
    // sync-provider's canonical resource-URL composition.
    QUrl initiateUrl(m_serverBaseUrl);
    QString basePath = initiateUrl.path();
    initiateUrl.setPath(basePath + QStringLiteral("/index.php/login/v2"), QUrl::DecodedMode);

    QNetworkRequest req(initiateUrl);
    // User-Agent identifies KeePassXC to the Nextcloud server.
    req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);
    // Refuse to follow redirects automatically; we surface redirect
    // responses to the user as failures rather than silently chasing them
    // (defense-in-depth against open-redirect-style auth detours).
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    // Empty body is acceptable; Nextcloud's Login Flow v2 initiate spec
    // sends no payload (Content-Length: 0).
    QNetworkReply* reply = m_nam->post(req, QByteArray());

    {
        QMutexLocker lock(&m_replyMutex);
        m_activeReply = reply;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onInitiateFinished(reply); });
}

void NextcloudLoginFlow::cancel()
{
    // Thread-safe cancel mirrors NextcloudSyncProvider::abort verbatim shape.
    // Stop both timers first (idempotent), then marshal abort() to the
    // reply's owning thread under the mutex. The reply's own finished()
    // handler observes the cancel via either OperationCanceledError (which
    // onPollFinished silently absorbs) or the QPointer becoming null
    // (already-deleted reply).
    m_pollTimer->stop();
    m_timeoutTimer->stop();

    {
        QMutexLocker locker(&m_replyMutex);
        if (m_activeReply) {
            QMetaObject::invokeMethod(m_activeReply, "abort", Qt::QueuedConnection);
        }
    }

    // Only transient states (Initiating / Polling) have an "active" flow to
    // cancel. Completed and Failed are terminal -- they already emitted
    // their respective signal, and re-emitting loginCancelled here would
    // undo a valid authorized UI state on dtor or on the cancel-previous
    // at the top of startLoginFlow. Idle has nothing to cancel.
    if (m_state == State::Initiating || m_state == State::Polling) {
        m_state = State::Idle;
        emit loginCancelled();
    }
}

void NextcloudLoginFlow::onInitiateFinished(QNetworkReply* reply)
{
    // Standard Qt idiom: defer reply destruction so any further signal
    // emissions on the reply (e.g. from Mock) finish first.
    reply->deleteLater();

    {
        QMutexLocker lock(&m_replyMutex);
        if (m_activeReply == reply) {
            m_activeReply.clear();
        }
    }

    // OperationCanceledError comes from cancel()/abort() during initiate.
    // cancel() already emitted loginCancelled and moved the state to Idle;
    // emitting loginFailed here would be a second terminal signal for the
    // same user action. Mirrors onPollFinished's absorber.
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    // Network error -> initiate-failure banner. Same banner text
    // regardless of which sub-case fails (network, malformed JSON, missing
    // fields).
    if (reply->error() != QNetworkReply::NoError) {
        emitFailureWithBanner(tr(BANNER_INITIATE_FAIL));
        return;
    }

    const QByteArray body = reply->readAll();

    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emitFailureWithBanner(tr(BANNER_INITIATE_FAIL));
        return;
    }

    const QJsonObject root = doc.object();
    const QString loginUrlStr = root.value(QStringLiteral("login")).toString();
    const QJsonObject pollObj = root.value(QStringLiteral("poll")).toObject();
    const QString pollToken = pollObj.value(QStringLiteral("token")).toString();
    const QString pollEndpointStr = pollObj.value(QStringLiteral("endpoint")).toString();

    if (loginUrlStr.isEmpty() || pollToken.isEmpty() || pollEndpointStr.isEmpty()) {
        emitFailureWithBanner(tr(BANNER_INITIATE_FAIL));
        return;
    }

    // Phishing mitigation: validate that the server-provided loginUrl AND
    // pollEndpoint scheme + host + port match the configured server URL. Per
    // Nextcloud server issue 21698 the server can return a loginUrl with an
    // attacker-controlled origin; opening it in the user's browser would phish
    // their credentials. The pollEndpoint flows from the same untrusted JSON
    // and would otherwise let the server downgrade the polling channel to
    // http:// (sending the app-password over cleartext) -- check it too.
    const QUrl loginUrl(loginUrlStr);
    const QUrl pollEndpoint(pollEndpointStr);
    const QUrl configuredUrl(m_serverBaseUrl);
    if (!hostsAndPortsMatch(loginUrl, configuredUrl) || !hostsAndPortsMatch(pollEndpoint, configuredUrl)) {
        emitFailureWithBanner(tr(BANNER_PHISHING));
        return;
    }

    // Stash for the polling state machine.
    m_pollToken = pollToken;
    m_pollEndpoint = pollEndpoint;

    // Pre-set Polling state so signal ordering reflects the post-initiate
    // lifecycle. The actual timer start happens in startPolling() below.
    m_state = State::Polling;

    emit loginInitiated(loginUrl);
    m_browserOpener(loginUrl);

    // Kick the poll loop. startPolling fires onPollTick once immediately so
    // the user does not wait one full poll interval before the first
    // attempt; subsequent ticks come from m_pollTimer (repeating).
    startPolling();
}

bool NextcloudLoginFlow::hostsAndPortsMatch(const QUrl& a, const QUrl& b)
{
    // Scheme equality is the first gate. Without it, a configured
    // https://cloud.example.com would accept a server-returned
    // http://cloud.example.com/... -- the host+port still match but the
    // browser would open an unencrypted URL, letting a network attacker
    // observe the polling token (loginUrl) or app-password (pollEndpoint).
    if (a.scheme() != b.scheme()) {
        return false;
    }
    if (a.host(QUrl::EncodeUnicode) != b.host(QUrl::EncodeUnicode)) {
        return false;
    }
    const int defaultPort = (a.scheme() == QStringLiteral("https")) ? 443 : 80;
    return a.port(defaultPort) == b.port(defaultPort);
}

void NextcloudLoginFlow::emitFailureWithBanner(const QString& bannerText)
{
    m_state = State::Failed;
    emit loginFailed(bannerText);
}

// ---------------------------------------------------------------------------
// Polling state machine.
// ---------------------------------------------------------------------------

void NextcloudLoginFlow::startPolling()
{
    m_state = State::Polling;

    // Reset interval here (in case setPollIntervalMsForTest was called between
    // ctor and startLoginFlow, which is the common test pattern).
    m_pollTimer->setInterval(m_pollIntervalMs);
    m_pollTimer->start();
    m_timeoutTimer->start(m_pollTimeoutMs);

    // Fire the first poll immediately so the user doesn't wait one full
    // interval (5 seconds in production) before any progress visible to the
    // UI.
    onPollTick();
}

void NextcloudLoginFlow::stopPollingTimers()
{
    m_pollTimer->stop();
    m_timeoutTimer->stop();
    QMutexLocker locker(&m_replyMutex);
    m_activeReply.clear();
}

void NextcloudLoginFlow::onPollTick()
{
    // Two guards against overlapping requests:
    //   * Skip when the flow already reached a terminal state (Completed /
    //     Failed / Cancelled). The timer can fire one more tick after
    //     stopPollingTimers() if a tick was already queued.
    //   * Skip when the previous reply is still in flight. A slow server can
    //     hold a reply longer than m_pollIntervalMs; without this guard each
    //     tick would stack a new POST on top, producing concurrent replies
    //     and racing terminal-signal emissions in onPollFinished.
    if (m_state != State::Polling) {
        return;
    }
    {
        QMutexLocker locker(&m_replyMutex);
        if (m_activeReply) {
            return;
        }
    }
    QNetworkRequest req(m_pollEndpoint);
    // User-Agent identifies KeePassXC; mirrors the initiate request.
    req.setRawHeader("User-Agent", QByteArray("KeePassXC/") + KEEPASSXC_VERSION);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    // ManualRedirectPolicy MUST be set per-request (the attribute is not
    // QNAM-wide; setting it on the QNAM has no effect on requests built
    // from a fresh QNetworkRequest). 3xx responses on the poll path are
    // state-machine-meaningful (e.g. 303 = 2FA per nextcloud/server#32689),
    // so silently chasing redirects would break the polling contract.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    QByteArray body = QByteArray("token=") + QUrl::toPercentEncoding(m_pollToken);
    QNetworkReply* reply = m_nam->post(req, body);

    {
        QMutexLocker locker(&m_replyMutex);
        m_activeReply = reply;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onPollFinished(reply); });
}

void NextcloudLoginFlow::onPollFinished(QNetworkReply* reply)
{
    // Standard Qt idiom: defer reply destruction so any further signal
    // emissions on the reply finish first. Mirrors onInitiateFinished.
    reply->deleteLater();

    {
        QMutexLocker locker(&m_replyMutex);
        if (m_activeReply == reply) {
            m_activeReply.clear();
        }
    }

    // OperationCanceledError comes from cancel()/abort() -- the cancel path has
    // already torn down state and emitted loginCancelled. Silently absorb so we
    // don't emit a second terminal signal.
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    // State guard: if a concurrent reply (or the timeout timer) already drove
    // the flow to a terminal state, swallow this late reply rather than
    // emitting a second loginSucceeded / loginFailed. Combined with the
    // overlap guard in onPollTick, this absorbs the rare race where two
    // replies were already in flight before the guard landed.
    if (m_state != State::Polling) {
        return;
    }

    // Network-level error (DNS resolution failure, connection refused) is a
    // hard failure: stop polling and emit the network-error banner. NOT a 4xx/5xx HTTP status
    // (those come back via QNetworkReply::NoError + the HttpStatusCodeAttribute).
    if (reply->error() != QNetworkReply::NoError) {
        const int httpStatusOnError = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatusOnError == 0) {
            stopPollingTimers();
            m_state = State::Failed;
            emit loginFailed(tr(BANNER_NETWORK_ERROR));
            return;
        }
        // else: HTTP status is set -- fall through to status-code dispatch.
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Still-polling statuses:
    //   3xx (any: 301/302/303/307/308) -- nextcloud/server#32689 (2FA returns 303)
    //   404 -- nextcloud-desktop flow2auth.cpp silent-ignore alignment
    //   410 -- per Nextcloud Login Flow v2 spec
    if ((httpStatus >= 300 && httpStatus < 400) || httpStatus == 404 || httpStatus == 410) {
        return;
    }

    if (httpStatus == 200) {
        // Parse JSON; require all 3 non-empty keys. Server defects that
        // return 200 with missing-key OR empty-value are surfaced through
        // the same network-error banner path as a genuine network error -- the user sees
        // "lost connection", which is the right user-facing characterization
        // even though the underlying cause is a server-side malformed
        // payload.
        const QByteArray body = reply->readAll();
        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            stopPollingTimers();
            m_state = State::Failed;
            emit loginFailed(tr(BANNER_NETWORK_ERROR));
            return;
        }
        const QJsonObject root = doc.object();
        const QString server = root.value(QStringLiteral("server")).toString();
        const QString loginName = root.value(QStringLiteral("loginName")).toString();
        const QString appPassword = root.value(QStringLiteral("appPassword")).toString();
        if (server.isEmpty() || loginName.isEmpty() || appPassword.isEmpty()) {
            stopPollingTimers();
            m_state = State::Failed;
            emit loginFailed(tr(BANNER_NETWORK_ERROR));
            return;
        }
        stopPollingTimers();
        m_state = State::Completed;
        emit loginCompleted(loginName, appPassword);
        return;
    }

    // Any other status (401, 403, 5xx, other 4xx like 400/418): hard failure.
    stopPollingTimers();
    m_state = State::Failed;
    emit loginFailed(tr(BANNER_NETWORK_ERROR));
}

void NextcloudLoginFlow::onPollTimeoutFired()
{
    // 5-minute hard cap reached. Stop everything and surface the timeout banner.
    stopPollingTimers();
    m_state = State::Failed;
    emit loginFailed(tr(BANNER_TIMEOUT));
}
