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

#include "TestOAuthHttpServer.h"
#include "remotesync/OAuthHttpServer.h"

#include <QEventLoop>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>

QTEST_GUILESS_MAIN(TestOAuthHttpServer)

namespace
{
    // Connects to 127.0.0.1:port, writes requestBytes, accumulates response
    // bytes via signal-driven event loop, exits when the server closes the
    // connection. Returns true if any response bytes were received.
    //
    // Why signal-driven instead of waitForReadyRead in a loop: same-thread
    // server means the response can land in our recv buffer between our
    // write() and the first waitForReadyRead(), and waitForReadyRead blocks
    // for NEW data only -- it then times out, costing seconds per test.
    bool sendRequest(quint16 port,
                     const QByteArray& requestBytes,
                     QByteArray* responseOut = nullptr,
                     int timeoutMs = 3000)
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, port);
        if (!socket.waitForConnected(timeoutMs)) {
            return false;
        }

        QByteArray response;
        QEventLoop loop;
        QObject::connect(&socket, &QTcpSocket::readyRead, &loop, [&]() {
            response.append(socket.readAll());
        });
        QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);

        socket.write(requestBytes);
        loop.exec();

        // Drain anything queued after the loop exited.
        response.append(socket.readAll());

        if (responseOut) {
            *responseOut = response;
        }
        return !response.isEmpty();
    }
} // namespace

void TestOAuthHttpServer::testStartStop()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));
    QVERIFY(server.isListening());
    QVERIFY(server.port() != 0);

    server.stop();
    QVERIFY(!server.isListening());
}

void TestOAuthHttpServer::testStartFailsOnPortConflict()
{
    // Occupy an OS-assigned port with a plain QTcpServer, then try to bind
    // OAuthHttpServer to the same port. SO_REUSEADDR semantics differ across
    // platforms, so on Windows this is reliable; on Linux the same exact port
    // typically also fails for a second listen() without SO_REUSEPORT. If the
    // OS surprises us and lets the second listen succeed, we skip rather than
    // pretend the conflict-detection logic is broken.
    QTcpServer blocker;
    QVERIFY(blocker.listen(QHostAddress::LocalHost, 0));
    quint16 takenPort = blocker.serverPort();

    OAuthHttpServer server;
    bool started = server.start(takenPort);
    if (started) {
        // Platform allowed the dual-bind; not a real test failure.
        server.stop();
        blocker.close();
        QSKIP("Platform allowed dual-bind of same port; conflict path not exercised");
    }
    QVERIFY(!server.isListening());
    blocker.close();
}

void TestOAuthHttpServer::testValidCodeCallback_emitsAuthCodeReceived()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /?code=ABC123 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(codeSpy.wait(2000) || codeSpy.count() == 1);
    QCOMPARE(codeSpy.count(), 1);
    QCOMPARE(codeSpy.takeFirst().at(0).toString(), QStringLiteral("ABC123"));
    QCOMPARE(errorSpy.count(), 0);

    QVERIFY(response.startsWith("HTTP/1.1 200"));
}

void TestOAuthHttpServer::testErrorCallback_emitsAuthError()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /?error=access_denied HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(errorSpy.wait(2000) || errorSpy.count() == 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).toString(), QStringLiteral("access_denied"));
    QCOMPARE(codeSpy.count(), 0);

    // Per source, server returns 200 even on user-decline -- the OAuth
    // callback URL is loaded by the user's browser and we want a clean tab.
    QVERIFY(response.startsWith("HTTP/1.1 200"));
}

void TestOAuthHttpServer::testStateMismatch_emits403AndAuthError()
{
    OAuthHttpServer server;
    server.setExpectedState(QStringLiteral("expected_state_value"));
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /?code=X&state=wrong_state HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(errorSpy.wait(2000) || errorSpy.count() == 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).toString(), QStringLiteral("state_mismatch"));
    QCOMPARE(codeSpy.count(), 0);

    QVERIFY(response.startsWith("HTTP/1.1 403"));
}

void TestOAuthHttpServer::testStateMatch_succeeds()
{
    OAuthHttpServer server;
    server.setExpectedState(QStringLiteral("expected_state_value"));
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /?code=X&state=expected_state_value HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(codeSpy.wait(2000) || codeSpy.count() == 1);
    QCOMPARE(codeSpy.count(), 1);
    QCOMPARE(codeSpy.takeFirst().at(0).toString(), QStringLiteral("X"));
    QCOMPARE(errorSpy.count(), 0);

    QVERIFY(response.startsWith("HTTP/1.1 200"));
}

void TestOAuthHttpServer::testNoCodeOrError_returns400()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /favicon.ico HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(response.startsWith("HTTP/1.1 400"));
    // Give any (unexpected) signal a moment to fire before asserting absence.
    QTest::qWait(100);
    QCOMPARE(codeSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
}

void TestOAuthHttpServer::testOversizedRequest_returns413()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    // MaxRequestSize is 8192. Build a request with a header value that pushes
    // the total over the limit, ensuring the server hits the 413 branch.
    QByteArray request = "GET /?code=ABC HTTP/1.1\r\nHost: localhost\r\nX-Padding: ";
    request.append(QByteArray(9000, 'A'));
    request.append("\r\n\r\n");

    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response, 5000));

    QVERIFY(response.startsWith("HTTP/1.1 413"));
    QTest::qWait(100);
    QCOMPARE(codeSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
}

void TestOAuthHttpServer::testDoubleCode_secondIgnored()
{
    OAuthHttpServer server;
    QVERIFY(server.start(0));

    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);

    QByteArray firstRequest = "GET /?code=A HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray firstResponse;
    QVERIFY(sendRequest(server.port(), firstRequest, &firstResponse));
    QVERIFY(codeSpy.wait(2000) || codeSpy.count() == 1);
    QCOMPARE(codeSpy.count(), 1);
    QCOMPARE(codeSpy.takeFirst().at(0).toString(), QStringLiteral("A"));
    QVERIFY(firstResponse.startsWith("HTTP/1.1 200"));

    // Second valid callback -- the m_codeReceived guard must suppress it.
    QByteArray secondRequest = "GET /?code=B HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray secondResponse;
    QVERIFY(sendRequest(server.port(), secondRequest, &secondResponse));
    QTest::qWait(100);
    QCOMPARE(codeSpy.count(), 0); // no NEW emissions since takeFirst above
    QVERIFY(secondResponse.startsWith("HTTP/1.1 200"));
}

void TestOAuthHttpServer::testStateClearedAfterStop()
{
    OAuthHttpServer server;
    server.setExpectedState(QStringLiteral("some_state"));
    QVERIFY(server.start(0));
    server.stop();

    // Restart and send a code WITHOUT a state parameter. If stop() correctly
    // cleared m_expectedState, the request succeeds. If state lingered, the
    // server would 403 with state_mismatch.
    QVERIFY(server.start(0));
    QSignalSpy codeSpy(&server, &OAuthHttpServer::authCodeReceived);
    QSignalSpy errorSpy(&server, &OAuthHttpServer::authError);

    QByteArray request = "GET /?code=X HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QByteArray response;
    QVERIFY(sendRequest(server.port(), request, &response));

    QVERIFY(codeSpy.wait(2000) || codeSpy.count() == 1);
    QCOMPARE(codeSpy.count(), 1);
    QCOMPARE(codeSpy.takeFirst().at(0).toString(), QStringLiteral("X"));
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(response.startsWith("HTTP/1.1 200"));
}
