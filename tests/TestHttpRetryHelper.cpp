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

#include "TestHttpRetryHelper.h"

#include "crypto/Crypto.h"
#include "remotesync/HttpRetryHelper.h"

#include <QAtomicInt>
#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>
#include <QUrl>

QTEST_GUILESS_MAIN(TestHttpRetryHelper)

namespace
{

/**
 * Tiny canned HTTP/1.1 server. Each accepted connection is answered with the
 * next pre-queued raw response, then closed. If the queue runs dry, the
 * connection is closed without writing -- which surfaces as an error reply.
 */
class CannedHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit CannedHttpServer(QObject* parent = nullptr)
        : QObject(parent)
        , m_server(new QTcpServer(this))
    {
        connect(m_server, &QTcpServer::newConnection, this, &CannedHttpServer::onNewConnection);
    }

    bool start()
    {
        return m_server->listen(QHostAddress::LocalHost, 0);
    }

    void stop()
    {
        m_server->close();
    }

    quint16 port() const
    {
        return m_server->serverPort();
    }

    /**
     * Queue a canned response. status is the HTTP status code; headers are
     * extra header lines (each ending with \r\n, no Content-Length needed --
     * it's appended automatically); body is the response body.
     */
    void queueResponse(int status, const QByteArray& extraHeaders = {}, const QByteArray& body = {})
    {
        QByteArray reasonPhrase;
        switch (status) {
        case 200:
            reasonPhrase = "OK";
            break;
        case 429:
            reasonPhrase = "Too Many Requests";
            break;
        case 500:
            reasonPhrase = "Internal Server Error";
            break;
        default:
            reasonPhrase = "Status";
            break;
        }

        QByteArray response;
        response += "HTTP/1.1 " + QByteArray::number(status) + " " + reasonPhrase + "\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Connection: close\r\n";
        if (!extraHeaders.isEmpty()) {
            response += extraHeaders;
        }
        response += "\r\n";
        response += body;
        m_responses.append(response);
    }

    int connectionCount() const
    {
        return m_connectionCount;
    }

private slots:
    void onNewConnection()
    {
        while (m_server->hasPendingConnections()) {
            auto* socket = m_server->nextPendingConnection();
            ++m_connectionCount;

            // Read the request until we see the end of headers, then respond.
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                m_buffers[socket].append(socket->readAll());
                if (!m_buffers[socket].contains("\r\n\r\n")) {
                    return;
                }
                if (!m_responses.isEmpty()) {
                    QByteArray response = m_responses.takeFirst();
                    socket->write(response);
                    socket->flush();
                }
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                m_buffers.remove(socket);
                socket->deleteLater();
            });
        }
    }

private:
    QTcpServer* m_server;
    QList<QByteArray> m_responses;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    int m_connectionCount = 0;
};

} // namespace

#include "TestHttpRetryHelper.moc"

void TestHttpRetryHelper::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestHttpRetryHelper::testIsRetryable_table_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("retryable");

    // Success / redirect: never retry.
    QTest::newRow("200 OK") << 200 << false;
    QTest::newRow("201 Created") << 201 << false;
    QTest::newRow("204 No Content") << 204 << false;
    QTest::newRow("301 Moved") << 301 << false;
    QTest::newRow("304 Not Modified") << 304 << false;

    // 4xx: never retry EXCEPT 429.
    QTest::newRow("400 Bad Request") << 400 << false;
    QTest::newRow("401 Unauthorized") << 401 << false;
    QTest::newRow("403 Forbidden") << 403 << false;
    QTest::newRow("404 Not Found") << 404 << false;
    QTest::newRow("428 (just below 429)") << 428 << false;
    QTest::newRow("429 Too Many Requests") << 429 << true;
    QTest::newRow("430 (just above 429)") << 430 << false;
    QTest::newRow("499") << 499 << false;

    // 5xx: always retry.
    QTest::newRow("500 Internal Server Error") << 500 << true;
    QTest::newRow("502 Bad Gateway") << 502 << true;
    QTest::newRow("503 Service Unavailable") << 503 << true;
    QTest::newRow("504 Gateway Timeout") << 504 << true;
    QTest::newRow("599 (upper edge)") << 599 << true;

    // Outside HTTP range: never retry.
    QTest::newRow("0 (no status)") << 0 << false;
    QTest::newRow("600 (above 5xx)") << 600 << false;
}

void TestHttpRetryHelper::testIsRetryable_table()
{
    QFETCH(int, status);
    QFETCH(bool, retryable);
    QCOMPARE(HttpRetryHelper::isRetryable(status), retryable);
}

void TestHttpRetryHelper::testExecute_succeedsFirstAttempt()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    server.queueResponse(200);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.baseDelayMs = 1;

    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, nullptr);
    QVERIFY(reply != nullptr);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QCOMPARE(callCount, 1);
    reply->deleteLater();
}

void TestHttpRetryHelper::testExecute_retriesOn500ThenSucceeds()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    server.queueResponse(500);
    server.queueResponse(200);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.baseDelayMs = 1; // keep wall-clock low

    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, nullptr);
    QVERIFY(reply != nullptr);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QCOMPARE(callCount, 2);
    reply->deleteLater();
}

void TestHttpRetryHelper::testExecute_exhaustsRetriesAndReturnsLastFailure()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    // maxRetries=2 means up to 3 total attempts (initial + 2 retries).
    server.queueResponse(500);
    server.queueResponse(500);
    server.queueResponse(500);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    RetryPolicy policy;
    policy.maxRetries = 2;
    policy.baseDelayMs = 1;

    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, nullptr);
    QVERIFY(reply != nullptr); // must NOT be nullptr -- caller needs to see the last failure
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 500);
    QCOMPARE(callCount, 3); // 1 initial + 2 retries
    reply->deleteLater();
}

void TestHttpRetryHelper::testExecute_retryAfterCapTriggersImmediateFailure()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    // 429 with Retry-After: 9999s -- well beyond the 60s default cap.
    server.queueResponse(429, "Retry-After: 9999\r\n");
    // Sentinel: if the helper incorrectly retries, it'd consume this 200.
    server.queueResponse(200);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.baseDelayMs = 1;
    policy.maxRetryAfterSec = 60;

    QElapsedTimer wallClock;
    wallClock.start();
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, nullptr);
    qint64 elapsed = wallClock.elapsed();

    QVERIFY(reply != nullptr);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 429);
    QCOMPARE(callCount, 1);
    // Should have returned immediately, NOT slept 9999s.
    QVERIFY2(elapsed < 5000, qPrintable(QString("elapsed=%1ms").arg(elapsed)));
    reply->deleteLater();
}

void TestHttpRetryHelper::testExecute_abortFlagShortCircuits()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    // Queue a 200 sentinel; if helper ignores abort, it'd consume this.
    server.queueResponse(200);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    QAtomicInt abortFlag(1); // set before call

    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.baseDelayMs = 1;

    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, &abortFlag);
    QCOMPARE(callCount, 0);
    QVERIFY(reply == nullptr);
    QCOMPARE(server.connectionCount(), 0);
}

void TestHttpRetryHelper::testExecute_abortFlagDuringDelayBreaksOut()
{
    CannedHttpServer server;
    QVERIFY(server.start());
    server.queueResponse(500);
    // Sentinel 200: if helper proceeds with retry, callCount would be 2.
    server.queueResponse(200);

    QNetworkAccessManager nam;
    int callCount = 0;
    auto makeRequest = [&]() -> QNetworkReply* {
        ++callCount;
        QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/").arg(server.port())));
        return nam.get(req);
    };

    QAtomicInt abortFlag(0);
    // Flip the flag ~50ms into the call -- which is inside the ~500ms delay.
    QTimer::singleShot(50, [&]() { abortFlag.storeRelease(1); });

    RetryPolicy policy;
    policy.maxRetries = 3;
    policy.baseDelayMs = 500; // delay window long enough for the 50ms flip to land inside

    QElapsedTimer wallClock;
    wallClock.start();
    QNetworkReply* reply = HttpRetryHelper::execute(makeRequest, policy, 5000, &abortFlag);
    qint64 elapsed = wallClock.elapsed();

    QVERIFY(reply != nullptr);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 500);
    QCOMPARE(callCount, 1);
    // Bounded wall-clock: should have aborted well before a full retry cycle.
    QVERIFY2(elapsed < 1000, qPrintable(QString("elapsed=%1ms").arg(elapsed)));
    reply->deleteLater();
}
