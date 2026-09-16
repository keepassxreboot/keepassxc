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

#ifndef KEEPASSXC_NEXTCLOUDLOGINFLOW_H
#define KEEPASSXC_NEXTCLOUDLOGINFLOW_H

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QJsonObject;

/**
 * Nextcloud Login Flow v2 driver: browser handshake + polling + cancel-previous
 * semantics.
 *
 * Owns the initiate-POST -> browser handshake -> 5s polling -> token receipt
 * state machine. Mirrors OAuthHttpServer's separation pattern: a self-contained
 * auth class in src/remotesync/, owning its transport, exposing terminal
 * signals.
 */
class NextcloudLoginFlow : public QObject
{
    Q_OBJECT

public:
    explicit NextcloudLoginFlow(QObject* parent = nullptr);
    ~NextcloudLoginFlow() override;

    // Begin a Login Flow v2 handshake: POST initiate to
    // <base>/index.php/login/v2, validate the returned loginUrl host (phishing
    // mitigation), open it in the user's browser, and start polling for
    // credentials. Cancels any previously in-flight flow first.
    //
    // virtual so MockNextcloudLoginFlow can override and emit canned terminal
    // signals synchronously, mirroring DropboxLoginFlow::startAuthorization
    // (which is the virtual that MockDropboxLoginFlow overrides for the same
    // reason).
    virtual void startLoginFlow(const QString& serverBaseUrl);

    // Cancel any in-flight initiate or polling request and emit
    // loginCancelled. Safe to call from any thread; abort is marshalled to
    // the network thread via QueuedConnection. Idempotent on Idle state.
    //
    // virtual to symmetrically allow MockNextcloudLoginFlow to short-circuit
    // the cancel teardown -- the mock has no timers / NAM / active reply to
    // tear down, so a base-class cancel() would be a no-op anyway, but
    // virtualizing keeps the override site contract identical to startLoginFlow.
    virtual void cancel();

    // Test seam -- production uses the lazily-constructed default. Caller
    // retains ownership: the injected NAM must outlive this object, is never
    // delete-d or reparented by the setter, and calling with nullptr does
    // not free a previously-set NAM.
    void setNetworkAccessManager(QNetworkAccessManager* nam);

    // Test seam -- production uses QDesktopServices::openUrl. Empty
    // std::function is ignored.
    void setBrowserOpener(std::function<void(const QUrl&)> opener);

    // Test-only: override the 5-second polling interval / 5-minute polling
    // timeout. Production uses the constants below.
    void setPollIntervalMsForTest(int ms);
    void setTimeoutMsForTest(int ms);

    // Production polling interval: 5 seconds (server-side recommended
    // cadence). Production polling timeout: 5 minutes -- shorter than the
    // server's 20-minute token lifetime.
    static constexpr int PollIntervalMs = 5000;
    static constexpr int PollTimeoutMs = 300000;

signals:
    // Emitted once the initiate POST succeeds and host validation passes;
    // carries the loginUrl that was just opened in the user's browser.
    void loginInitiated(QUrl loginUrl);

    // Emitted on successful Login Flow v2 completion; payload is the
    // credentials the settings widget persists into the database's
    // CustomData.
    void loginCompleted(QString loginName, QString appPassword);

    // Emitted on hard failure (timeout, malformed body, host-validation
    // rejection, network error) -- carries the user-facing banner verbatim.
    void loginFailed(QString reason);

    // Emitted exactly once when cancel() succeeds in stopping an active flow.
    void loginCancelled();

private slots:
    // Handle the initiate POST response. Parses JSON, validates loginUrl
    // host, emits loginInitiated and opens the browser on success, or emits
    // loginFailed with the locked banner on failure.
    void onInitiateFinished(QNetworkReply* reply);

    // Fire one poll request. Connected to m_pollTimer::timeout (repeating)
    // and called once directly from startPolling so the user does not wait
    // one full poll interval before the first attempt.
    void onPollTick();

    // Handle a poll response. Dispatches by HTTP status: 200+full keys ->
    // loginCompleted, 200+missing/empty key -> failure banner, 3xx/404/410
    // -> continue polling, 401/403/5xx/other -> failure banner, network
    // error -> failure banner.
    void onPollFinished(QNetworkReply* reply);

    // m_timeoutTimer (single-shot 5-min) fired -- emit hard-timeout banner
    // and tear down both timers / active reply.
    void onPollTimeoutFired();

private:
    // State machine for the login flow lifecycle.
    enum class State
    {
        Idle,
        Initiating,
        Polling,
        Completed,
        Failed,
        Cancelled
    };

    // Lazy-construct or return the injected QNetworkAccessManager. Mirrors
    // NextcloudSyncProvider::ensureNam.
    void ensureNam();

    // Phishing mitigation: scheme equality, then case-sensitive host
    // comparison on the EncodeUnicode form (handles IDN), then port comparison
    // falling back to the scheme default (443/https, 80/http). Scheme must
    // match because otherwise an https-configured server could return an
    // http://<same-host>/ login URL or poll endpoint and downgrade the
    // channel. Returns true iff both origins match.
    static bool hostsAndPortsMatch(const QUrl& a, const QUrl& b);

    // DRY emit helper: transition to Failed and emit
    // loginFailed(bannerText).
    void emitFailureWithBanner(const QString& bannerText);

    // Kick off the poll loop after a successful initiate. Sets m_state =
    // Polling, starts m_pollTimer (repeating m_pollIntervalMs) and
    // m_timeoutTimer (single-shot m_pollTimeoutMs), then fires onPollTick()
    // directly so the first poll does not wait one full interval.
    void startPolling();

    // Stop both timers and clear m_activeReply under m_replyMutex. Called
    // from terminal branches in onPollFinished, onPollTimeoutFired, and
    // cancel(). Idempotent.
    void stopPollingTimers();

    QNetworkAccessManager* m_nam = nullptr;

    std::function<void(const QUrl&)> m_browserOpener;

    int m_pollIntervalMs = PollIntervalMs;
    int m_pollTimeoutMs = PollTimeoutMs;

    QString m_serverBaseUrl;
    QString m_pollToken;
    QUrl m_pollEndpoint;

    QPointer<QNetworkReply> m_activeReply;
    mutable QMutex m_replyMutex;

    QTimer* m_pollTimer = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    State m_state = State::Idle;

    Q_DISABLE_COPY(NextcloudLoginFlow)
};

#endif // KEEPASSXC_NEXTCLOUDLOGINFLOW_H
