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

#include "OAuthHttpServer.h"

#include "HttpStatus.h"

#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace
{
    const char* successHtml = "<html><body>"
                              "<h2>Authorization successful.</h2>"
                              "<p>You can close this tab and return to KeePassXC.</p>"
                              "</body></html>";

    const char* errorHtml = "<html><body>"
                            "<h2>Authorization failed.</h2>"
                            "<p>Error: %1</p>"
                            "<p>Please close this tab and try again in KeePassXC.</p>"
                            "</body></html>";

    const char* invalidRequestHtml = "<html><body>"
                                     "<h2>Invalid request.</h2>"
                                     "</body></html>";

    QByteArray buildHttpResponse(int statusCode, const QString& statusText, const QString& body)
    {
        QByteArray response;
        response.append(QStringLiteral("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
        response.append("Content-Type: text/html; charset=utf-8\r\n");
        response.append("Connection: close\r\n");
        response.append("\r\n");
        response.append(body.toUtf8());
        return response;
    }
} // namespace

OAuthHttpServer::OAuthHttpServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &OAuthHttpServer::handleNewConnection);
}

OAuthHttpServer::~OAuthHttpServer()
{
    stop();
}

bool OAuthHttpServer::start(quint16 port)
{
    m_codeReceived = false;
    return m_server->listen(QHostAddress::LocalHost, port);
}

quint16 OAuthHttpServer::port() const
{
    return m_server->serverPort();
}

void OAuthHttpServer::stop()
{
    m_server->close();
    m_codeReceived = false;
    m_expectedState.clear();

    // Close any still-connected sockets to prevent dangling lambda captures
    const auto sockets = m_server->findChildren<QTcpSocket*>();
    for (auto* socket : sockets) {
        socket->disconnectFromHost();
    }
}

void OAuthHttpServer::setExpectedState(const QString& state)
{
    m_expectedState = state;
}

bool OAuthHttpServer::isListening() const
{
    return m_server->isListening();
}

void OAuthHttpServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        auto* socket = m_server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        // Per-socket read timeout to prevent slowloris-style attacks
        QTimer::singleShot(SocketTimeoutMs, socket, [socket]() {
            if (socket->state() != QTcpSocket::UnconnectedState) {
                socket->disconnectFromHost();
            }
        });

        // Accumulate data until we have complete HTTP headers
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            if (socket->bytesAvailable() > 0) {
                auto data = socket->property("_httpBuffer").toByteArray();
                data.append(socket->readAll());

                // Reject oversized requests to prevent unbounded memory usage
                if (data.size() > MaxRequestSize) {
                    socket->write(buildHttpResponse(413, "Payload Too Large", invalidRequestHtml));
                    socket->flush();
                    // Disconnect after the write buffer is flushed to the client
                    connect(socket, &QTcpSocket::bytesWritten, socket, [socket]() {
                        if (socket->bytesToWrite() == 0) {
                            socket->disconnectFromHost();
                        }
                    });
                    return;
                }

                socket->setProperty("_httpBuffer", data);

                if (data.contains("\r\n\r\n")) {
                    processRequest(socket);
                }
            }
        });

        // Clean up socket on disconnect
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void OAuthHttpServer::processRequest(QTcpSocket* socket)
{
    auto requestData = socket->property("_httpBuffer").toByteArray();
    auto requestLine = QString::fromUtf8(requestData.left(requestData.indexOf("\r\n")));

    // Parse the request line: "GET /path?query HTTP/1.1"
    auto parts = requestLine.split(' ');
    if (parts.size() < 2) {
        socket->write(buildHttpResponse(HttpStatus::BadRequest, "Bad Request", invalidRequestHtml));
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    auto urlStr = parts.at(1);
    QUrl url(urlStr);
    QUrlQuery query(url.query());

    // Only process if we have a code or error parameter
    QString code = query.queryItemValue("code");
    QString error = query.queryItemValue("error");

    if (code.isEmpty() && error.isEmpty()) {
        // Stray browser request (e.g. favicon.ico): respond 400 and close.
        socket->write(buildHttpResponse(HttpStatus::BadRequest, "Bad Request", invalidRequestHtml));
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    // Validate OAuth state parameter for CSRF protection (RFC 6749 §10.12)
    if (!m_expectedState.isEmpty()) {
        QString receivedState = query.queryItemValue("state");
        if (receivedState != m_expectedState) {
            socket->write(buildHttpResponse(
                HttpStatus::Forbidden,
                "Forbidden",
                QString(errorHtml).arg(QStringLiteral("State mismatch - possible CSRF attack").toHtmlEscaped())));
            socket->flush();
            socket->disconnectFromHost();
            emit authError(QStringLiteral("state_mismatch"));
            return;
        }
    }

    // Prevent double-processing
    if (m_codeReceived) {
        socket->write(buildHttpResponse(HttpStatus::Ok, "OK", successHtml));
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    m_codeReceived = true;

    if (!code.isEmpty()) {
        socket->write(buildHttpResponse(HttpStatus::Ok, "OK", successHtml));
        socket->flush();
        socket->disconnectFromHost();
        emit authCodeReceived(code);
    } else {
        auto errorDescription = query.queryItemValue("error_description").replace('+', ' ');
        auto errorMsg = errorDescription.isEmpty() ? error : errorDescription;
        socket->write(buildHttpResponse(HttpStatus::Ok, "OK", QString(errorHtml).arg(errorMsg.toHtmlEscaped())));
        socket->flush();
        socket->disconnectFromHost();
        emit authError(error);
    }
}
