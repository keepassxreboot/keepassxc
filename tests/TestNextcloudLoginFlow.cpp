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

#include "TestNextcloudLoginFlow.h"
#include "remotesync/NextcloudLoginFlow.h"

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>
#include <QUrl>

QTEST_GUILESS_MAIN(TestNextcloudLoginFlow)

namespace
{
    // -----------------------------------------------------------------------
    // FakeNextcloudServer: a tiny in-process HTTP server that speaks just
    // enough HTTP/1.1 to satisfy QNetworkAccessManager. Per-path canned
    // responses are queued; each incoming request pops the next response for
    // the matching path. Each request URL path is also recorded for later
    // assertion.
    // -----------------------------------------------------------------------
    struct CannedResponse
    {
        int statusCode = 200;
        QByteArray body;
        // If true, instead of replying, close the socket immediately after
        // receiving the request (for "network error" tests). NOTE: this
        // currently produces a closed-without-response which the client
        // surfaces as a QNetworkReply error.
        bool dropConnection = false;
        // If non-zero, delay the reply by this many ms before sending bytes.
        // Used by cancel-during-* tests to keep the reply pending.
        int delayMs = 0;
    };

    class FakeNextcloudServer : public QObject
    {
        Q_OBJECT
    public:
        explicit FakeNextcloudServer(QObject* parent = nullptr)
            : QObject(parent)
        {
            connect(&m_server, &QTcpServer::newConnection, this, &FakeNextcloudServer::onNewConnection);
        }

        bool listen()
        {
            return m_server.listen(QHostAddress::LocalHost, 0);
        }

        quint16 port() const
        {
            return m_server.serverPort();
        }

        QString baseUrl() const
        {
            return QStringLiteral("http://127.0.0.1:%1").arg(port());
        }

        // Queue a canned response. path is the URL path (e.g. "/index.php/login/v2").
        void queueResponse(const QString& path, const CannedResponse& r)
        {
            m_responses[path].append(r);
        }

        // Default response for any path that has no queued response: 404.
        // Useful for "keep polling on 404" — queue only 1 200 and let the
        // server backfill 404s.
        void setDefaultResponse(const QString& path, const CannedResponse& r)
        {
            m_defaults[path] = r;
        }

        QStringList requestedPaths() const
        {
            return m_requestedPaths;
        }

        int requestCountFor(const QString& path) const
        {
            int n = 0;
            for (const QString& p : m_requestedPaths) {
                if (p == path) {
                    ++n;
                }
            }
            return n;
        }

    private slots:
        void onNewConnection()
        {
            while (m_server.hasPendingConnections()) {
                QTcpSocket* sock = m_server.nextPendingConnection();
                connect(sock, &QTcpSocket::readyRead, this, [this, sock]() { onReadyRead(sock); });
                connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
            }
        }

        void onReadyRead(QTcpSocket* sock)
        {
            // Accumulate per-socket buffer until we have the full request
            // headers + (for POST) the declared body. Nextcloud Login Flow v2
            // initiate is Content-Length: 0; the poll POST sends a small
            // form-urlencoded body. We use Content-Length to detect end of
            // request rather than chase chunked encoding (QNAM doesn't send
            // chunked for these small bodies in practice).
            m_buffers[sock].append(sock->readAll());

            QByteArray& buf = m_buffers[sock];
            int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return; // wait for more bytes
            }

            QByteArray headerPart = buf.left(headerEnd);
            // Find Content-Length if present.
            int contentLength = 0;
            for (const QByteArray& line : headerPart.split('\n')) {
                if (line.toLower().startsWith("content-length:")) {
                    contentLength = line.mid(QByteArray("content-length:").size()).trimmed().toInt();
                    break;
                }
            }
            if (buf.size() < headerEnd + 4 + contentLength) {
                return; // wait for body
            }

            // Extract request line: "METHOD PATH HTTP/1.1"
            QByteArray requestLine = headerPart.split('\n').value(0).trimmed();
            QList<QByteArray> parts = requestLine.split(' ');
            QString path;
            if (parts.size() >= 2) {
                path = QString::fromLatin1(parts.at(1));
            }
            m_requestedPaths.append(path);

            // Pick the response: queued first, then default, then 404.
            CannedResponse resp;
            bool haveResp = false;
            if (m_responses.contains(path) && !m_responses[path].isEmpty()) {
                resp = m_responses[path].takeFirst();
                haveResp = true;
            } else if (m_defaults.contains(path)) {
                resp = m_defaults.value(path);
                haveResp = true;
            }
            if (!haveResp) {
                resp.statusCode = 404;
                resp.body = QByteArray();
            }

            // Clear consumed buffer so a pipelined second request on the
            // same socket would parse from a clean slate. In practice QNAM
            // closes after one POST so this is defensive.
            m_buffers[sock] = buf.mid(headerEnd + 4 + contentLength);

            auto sendReply = [sock, resp]() {
                if (sock->state() != QAbstractSocket::ConnectedState) {
                    return;
                }
                if (resp.dropConnection) {
                    sock->abort();
                    return;
                }
                QByteArray reply;
                reply.append("HTTP/1.1 ").append(QByteArray::number(resp.statusCode)).append(" X\r\n");
                reply.append("Content-Type: application/json\r\n");
                reply.append("Content-Length: ").append(QByteArray::number(resp.body.size())).append("\r\n");
                reply.append("Connection: close\r\n\r\n");
                reply.append(resp.body);
                sock->write(reply);
                sock->disconnectFromHost();
            };

            if (resp.delayMs > 0) {
                QTimer::singleShot(resp.delayMs, sock, sendReply);
            } else {
                sendReply();
            }
        }

    private:
        QTcpServer m_server;
        QMap<QString, QList<CannedResponse>> m_responses;
        QMap<QString, CannedResponse> m_defaults;
        QStringList m_requestedPaths;
        QMap<QTcpSocket*, QByteArray> m_buffers;
    };

    // Locate a free localhost port that has NO listener — used for
    // "network error" testing. Bind a server, capture its port, close it,
    // and return the now-vacated port. There is a (very small) race window
    // where another process could grab it; CI is single-tenant enough that
    // this is acceptable.
    quint16 findUnusedLocalPort()
    {
        QTcpServer tmp;
        tmp.listen(QHostAddress::LocalHost, 0);
        quint16 p = tmp.serverPort();
        tmp.close();
        return p;
    }

    // Make a canned 200 JSON response.
    CannedResponse jsonOk(const QByteArray& body)
    {
        CannedResponse r;
        r.statusCode = 200;
        r.body = body;
        return r;
    }

    CannedResponse statusOnly(int status)
    {
        CannedResponse r;
        r.statusCode = status;
        r.body = QByteArray();
        return r;
    }
} // namespace

#include "TestNextcloudLoginFlow.moc"

// ----------------------------------------------------------------------------
// Happy path
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testHappyPath_initiateThenPollSucceeds()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    // Initiate: 200 with login/poll structure.
    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    // Poll: 200 with credential triple.
    QByteArray pollBody =
        QStringLiteral("{\"server\":\"%1\",\"loginName\":\"alice\",\"appPassword\":\"app-pwd-xyz\"}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/poll"), jsonOk(pollBody));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);

    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);
    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);

    flow.startLoginFlow(base);

    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(initSpy.count(), 1);
    QCOMPARE(initSpy.takeFirst().at(0).toUrl(), QUrl(base + QStringLiteral("/login")));

    QCOMPARE(openedUrls.size(), 1);
    QCOMPARE(openedUrls.at(0), QUrl(base + QStringLiteral("/login")));

    QCOMPARE(completedSpy.count(), 1);
    QList<QVariant> args = completedSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("alice"));
    QCOMPARE(args.at(1).toString(), QStringLiteral("app-pwd-xyz"));

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(cancelledSpy.count(), 0);
}

// ----------------------------------------------------------------------------
// Phishing mitigation
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testPhishing_loginUrlHostMismatch_failsBeforePolling()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"https://evil.com/login\",\"poll\":{\"token\":\"x\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);

    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    const QString reason = failedSpy.takeFirst().at(0).toString();
    QVERIFY2(reason.contains(QStringLiteral("unexpected")) || reason.contains(QStringLiteral("Verify your server URL")),
             qPrintable(reason));

    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);

    // Make sure no poll request was ever sent. The flow could only have
    // touched the initiate path.
    QTest::qWait(50);
    QCOMPARE(server.requestCountFor(QStringLiteral("/poll")), 0);
}

void TestNextcloudLoginFlow::testPhishing_pollEndpointHostMismatch_fails()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    // loginUrl matches host, but pollEndpoint points to evil.com.
    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"x\",\"endpoint\":\"https://evil.com/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    NextcloudLoginFlow flow;
    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);
}

void TestNextcloudLoginFlow::testPhishing_pollEndpointSchemeMismatch_fails()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl(); // http://127.0.0.1:<port>

    // pollEndpoint scheme is https while configured base is http.
    QString httpsPoll = QStringLiteral("https://127.0.0.1:%1/poll").arg(server.port());
    QByteArray initiateBody = QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"x\",\"endpoint\":\"%2\"}}")
                                  .arg(base, httpsPoll)
                                  .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    NextcloudLoginFlow flow;
    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);
}

// ----------------------------------------------------------------------------
// Initiate failure paths
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testInitiate_networkError_emitsFailed()
{
    quint16 deadPort = findUnusedLocalPort();
    const QString base = QStringLiteral("http://127.0.0.1:%1").arg(deadPort);

    NextcloudLoginFlow flow;
    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);
}

void TestNextcloudLoginFlow::testInitiate_malformedJson_emitsFailed()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    CannedResponse r;
    r.statusCode = 200;
    r.body = QByteArray("not json");
    server.queueResponse(QStringLiteral("/index.php/login/v2"), r);

    NextcloudLoginFlow flow;
    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);
}

void TestNextcloudLoginFlow::testInitiate_missingFields_emitsFailed()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(QByteArray("{}")));

    NextcloudLoginFlow flow;
    QList<QUrl> openedUrls;
    flow.setBrowserOpener([&](const QUrl& u) { openedUrls.append(u); });

    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(openedUrls.size(), 0);
}

// ----------------------------------------------------------------------------
// Polling state machine
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testPolling_keepsPollingOn404()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    // First poll: 404. Second poll: 200 with creds.
    server.queueResponse(QStringLiteral("/poll"), statusOnly(404));
    QByteArray pollBody =
        QStringLiteral("{\"server\":\"%1\",\"loginName\":\"bob\",\"appPassword\":\"pw\"}").arg(base).toUtf8();
    server.queueResponse(QStringLiteral("/poll"), jsonOk(pollBody));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(completedSpy.takeFirst().at(0).toString(), QStringLiteral("bob"));
    QVERIFY(server.requestCountFor(QStringLiteral("/poll")) >= 2);
}

void TestNextcloudLoginFlow::testPolling_keepsPollingOn3xx()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));

    server.queueResponse(QStringLiteral("/poll"), statusOnly(303));
    QByteArray pollBody =
        QStringLiteral("{\"server\":\"%1\",\"loginName\":\"carol\",\"appPassword\":\"pw\"}").arg(base).toUtf8();
    server.queueResponse(QStringLiteral("/poll"), jsonOk(pollBody));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
}

void TestNextcloudLoginFlow::testPolling_hardFailureOn401()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));
    server.queueResponse(QStringLiteral("/poll"), statusOnly(401));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    const QString reason = failedSpy.takeFirst().at(0).toString();
    QVERIFY2(reason.contains(QStringLiteral("Lost connection")), qPrintable(reason));
}

void TestNextcloudLoginFlow::testPolling_hardFailureOn500()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));
    server.queueResponse(QStringLiteral("/poll"), statusOnly(500));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
}

void TestNextcloudLoginFlow::testPolling_timeoutFiresAfterPollTimeoutMs()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));
    // Every poll returns 404 indefinitely.
    server.setDefaultResponse(QStringLiteral("/poll"), statusOnly(404));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(200);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    QVERIFY(failedSpy.wait(2000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    const QString reason = failedSpy.takeFirst().at(0).toString();
    QVERIFY2(reason.contains(QStringLiteral("timed out")), qPrintable(reason));
}

// ----------------------------------------------------------------------------
// Cancel semantics
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testCancel_inIdle_isNoop()
{
    NextcloudLoginFlow flow;
    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);
    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);

    flow.cancel();
    QTest::qWait(50);

    QCOMPARE(cancelledSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(initSpy.count(), 0);
}

void TestNextcloudLoginFlow::testCancel_duringInitiate_emitsCancelled()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    // Initiate response is delayed long enough that cancel() arrives first.
    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    CannedResponse slow = jsonOk(initiateBody);
    slow.delayMs = 1000;
    server.queueResponse(QStringLiteral("/index.php/login/v2"), slow);

    NextcloudLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);
    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);

    flow.startLoginFlow(base);
    flow.cancel();

    QVERIFY(cancelledSpy.wait(2000) || cancelledSpy.count() == 1);
    QCOMPARE(cancelledSpy.count(), 1);

    // Give the delayed server reply a chance to arrive at the (already-aborted)
    // reply object — onInitiateFinished must NOT emit a second terminal signal.
    QTest::qWait(1500);
    QCOMPARE(initSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(cancelledSpy.count(), 1);
}

void TestNextcloudLoginFlow::testCancel_duringPolling_emitsCancelled()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));
    // Default poll: 404 forever — keeps the flow in Polling state.
    server.setDefaultResponse(QStringLiteral("/poll"), statusOnly(404));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy initSpy(&flow, &NextcloudLoginFlow::loginInitiated);
    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);
    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);

    flow.startLoginFlow(base);

    // Wait until polling is underway.
    QVERIFY(initSpy.wait(5000));

    flow.cancel();

    QVERIFY(cancelledSpy.wait(2000) || cancelledSpy.count() == 1);
    QCOMPARE(cancelledSpy.count(), 1);

    // No double-emit afterward.
    QTest::qWait(200);
    QCOMPARE(cancelledSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
}

void TestNextcloudLoginFlow::testCancel_afterCompleted_doesNotReEmit()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    server.queueResponse(QStringLiteral("/index.php/login/v2"), jsonOk(initiateBody));
    QByteArray pollBody =
        QStringLiteral("{\"server\":\"%1\",\"loginName\":\"u\",\"appPassword\":\"p\"}").arg(base).toUtf8();
    server.queueResponse(QStringLiteral("/poll"), jsonOk(pollBody));

    NextcloudLoginFlow flow;
    flow.setPollIntervalMsForTest(10);
    flow.setTimeoutMsForTest(5000);
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy completedSpy(&flow, &NextcloudLoginFlow::loginCompleted);
    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);

    flow.startLoginFlow(base);
    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(completedSpy.count(), 1);

    flow.cancel();
    QTest::qWait(50);
    QCOMPARE(cancelledSpy.count(), 0);
}

void TestNextcloudLoginFlow::testCancel_afterFailed_doesNotReEmit()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    CannedResponse bad;
    bad.statusCode = 200;
    bad.body = QByteArray("not json");
    server.queueResponse(QStringLiteral("/index.php/login/v2"), bad);

    NextcloudLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy failedSpy(&flow, &NextcloudLoginFlow::loginFailed);
    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);

    flow.startLoginFlow(base);
    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);

    flow.cancel();
    QTest::qWait(50);
    QCOMPARE(cancelledSpy.count(), 0);
}

// ----------------------------------------------------------------------------
// Cancel-previous on startLoginFlow
// ----------------------------------------------------------------------------

void TestNextcloudLoginFlow::testStartLoginFlow_cancelsPreviousFlow()
{
    FakeNextcloudServer server;
    QVERIFY(server.listen());
    const QString base = server.baseUrl();

    // First flow: slow initiate so it is still in-flight when we kick the second.
    QByteArray initiateBody =
        QStringLiteral("{\"login\":\"%1/login\",\"poll\":{\"token\":\"abc\",\"endpoint\":\"%1/poll\"}}")
            .arg(base)
            .toUtf8();
    CannedResponse slow = jsonOk(initiateBody);
    slow.delayMs = 1000;
    server.queueResponse(QStringLiteral("/index.php/login/v2"), slow);
    // Second flow's initiate: also slow so we can observe the cancellation
    // signal from the first flow without the second flow racing to a terminal.
    CannedResponse slow2 = jsonOk(initiateBody);
    slow2.delayMs = 2000;
    server.queueResponse(QStringLiteral("/index.php/login/v2"), slow2);

    NextcloudLoginFlow flow;
    flow.setBrowserOpener([](const QUrl&) {});

    QSignalSpy cancelledSpy(&flow, &NextcloudLoginFlow::loginCancelled);

    flow.startLoginFlow(base);
    // Allow the post to actually leave QNAM before re-entering startLoginFlow.
    QTest::qWait(50);
    flow.startLoginFlow(base);

    QVERIFY(cancelledSpy.wait(2000) || cancelledSpy.count() == 1);
    QCOMPARE(cancelledSpy.count(), 1);

    // Tear down the second flow cleanly so test exit doesn't churn the event loop.
    flow.cancel();
}
