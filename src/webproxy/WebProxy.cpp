/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WebProxy.h"
#include "browser/BrowserShared.h"

#include <QJsonDocument>
#include <QStandardPaths>
#include <QTextStream>
#include <QtWebSockets/qwebsocket.h>
#include <QtWebSockets/qwebsocketserver.h>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <winsock2.h>
#undef NOMINMAX
#define NOMINMAX
#include <windows.h>
#else
#include <sys/socket.h>
#endif
#if defined(Q_OS_MACOS)
#include <unistd.h>
#endif

const static auto WEBSOCKET_SERVER_NAME = QStringLiteral("KeePassXC WebSocket server");
const static auto LOCAL_SERVER_PATH = BrowserShared::localServerPath();
constexpr int DEFAULT_WEBSOCKET_PORT = 7580;

static QTextStream out(stdout);
static QTextStream err(stderr);

WebProxy::WebProxy(const QString& webSocketPort, QObject* parent)
    : QObject(parent)
{
    setupLocalSocket();
    setupWebSocketServer(webSocketPort);
}

WebProxy::~WebProxy()
{
    stopWebSocket();
}

void WebProxy::setupLocalSocket()
{
    connect(this, &WebProxy::webSocketMessageReceived, this, &WebProxy::sendToLocalSocket, Qt::QueuedConnection);

    m_localSocket.reset(new QLocalSocket());
    connect(m_localSocket.data(), &QLocalSocket::connected, this, &WebProxy::localSocketConnected);
    connect(m_localSocket.data(), &QLocalSocket::disconnected, this, &WebProxy::localSocketDisconnected);
    connect(m_localSocket.data(), &QLocalSocket::errorOccurred, this, &WebProxy::localSocketErrored);

    m_localSocket->connectToServer(LOCAL_SERVER_PATH);
    m_localSocket->setReadBufferSize(BrowserShared::SOCKET_BUFFER_SIZE);
    int socketDesc = m_localSocket->socketDescriptor();
    if (socketDesc) {
        int max = BrowserShared::SOCKET_BUFFER_SIZE;
        setsockopt(socketDesc, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&max), sizeof(max));
    }

    connect(m_localSocket.data(), SIGNAL(readyRead()), this, SLOT(readLocalSocketMessage()));
}

void WebProxy::readLocalSocketMessage()
{
    auto message = m_localSocket->readAll();
    if (!message.isEmpty()) {
        const auto jsonMessage = parseMessage(message);
        if (jsonMessage.isEmpty()) {
            return;
        }

        const auto clientID = jsonMessage["clientID"].toString();
        if (!clientID.isEmpty()) {
            // Send to specific clientID & socket
            const auto socket = m_socketList.value(clientID);
            if (socket) {
                sendToWebSocket(socket, message);
            }
            return;
        }

        // Broadcast to all WebSockets
        for (const auto socket : m_socketList.values()) {
            sendToWebSocket(socket, message);
        }
    }
}

void WebProxy::localSocketConnected()
{
    out << QObject::tr("Local socket connected to %1.").arg(LOCAL_SERVER_PATH) << Qt::endl;
}

void WebProxy::localSocketDisconnected()
{
    out << QObject::tr("Local socket disconnected") << Qt::endl;
}

void WebProxy::localSocketErrored(QLocalSocket::LocalSocketError socketError)
{
    err << QObject::tr("Local socket has an error: %1 %2")
               .arg(QString::number(socketError), m_localSocket->errorString())
        << Qt::endl;
}

void WebProxy::setupWebSocketServer(const QString& webSocketPort)
{
    const int socketPort = !webSocketPort.isEmpty() ? webSocketPort.toInt() : DEFAULT_WEBSOCKET_PORT;
    if (!m_webSocketServer) {
        m_webSocketServer.reset(new QWebSocketServer(WEBSOCKET_SERVER_NAME, QWebSocketServer::NonSecureMode, this));
    }

    if (!m_webSocketServer->isListening()) {
        m_webSocketServer->listen(QHostAddress::LocalHost, socketPort);
        out << QObject::tr("WebSocket listening to port %1").arg(socketPort) << Qt::endl;
        connect(m_webSocketServer.data(), &QWebSocketServer::newConnection, this, &WebProxy::webSocketConnected);
        connect(m_webSocketServer.data(), &QWebSocketServer::closed, this, [&] { m_webSocketServer->deleteLater(); });
        connect(m_webSocketServer.data(), &QWebSocketServer::serverError, this, &WebProxy::webSocketServerError);
    }
}

void WebProxy::stopWebSocket()
{
    m_socketList.clear();
    m_webSocketServer->close();
}

void WebProxy::webSocketConnected()
{
    auto socket = m_webSocketServer->nextPendingConnection();
    if (socket) {
        out << QObject::tr("New connection from WebSocket") << Qt::endl;
        connect(socket, &QWebSocket::textMessageReceived, this, &WebProxy::readWebSocketMessage);
        connect(socket, &QWebSocket::disconnected, this, &WebProxy::webSocketDisconnected);
    }
}

void WebProxy::readWebSocketMessage(QString message)
{
    auto* socket = qobject_cast<QWebSocket*>(QObject::sender());
    if (!socket || !socket->isValid()) {
        return;
    }

    socket->setReadBufferSize(BrowserShared::SOCKET_BUFFER_SIZE);
    socket->setOutgoingFrameSize(BrowserShared::SOCKET_BUFFER_SIZE);

    const auto jsonMessage = parseMessage(message);
    if (jsonMessage.isEmpty()) {
        return;
    }

    // If no socket is created for this clientID, store the clientID and socket to list
    const auto storedKey = m_socketList.key(socket);
    if (storedKey.isEmpty()) {
        const auto clientID = jsonMessage["clientID"].toString();
        if (!clientID.isEmpty()) {
            out << QObject::tr("Client ID %1 connected").arg(clientID) << Qt::endl;
            m_socketList.insert(clientID, socket);
        } else {
            err << QObject::tr("No clientID provided in message.") << Qt::endl;
        }
    }

    emit webSocketMessageReceived(socket, jsonMessage);
}

void WebProxy::sendToWebSocket(QWebSocket* socket, const QString& data)
{
    if (socket && socket->isValid() && socket->state() == QAbstractSocket::ConnectedState) {
        socket->sendTextMessage(data);
        socket->flush();
    }
}

void WebProxy::sendToLocalSocket(QWebSocket* socket, const QJsonObject& json)
{
    Q_UNUSED(socket)
    const QString msg(QJsonDocument(json).toJson(QJsonDocument::Compact));
    if (m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState) {
        m_localSocket->write(msg.toUtf8(), msg.length());
        m_localSocket->flush();
    }
}

void WebProxy::webSocketDisconnected()
{
    const auto socket = qobject_cast<QWebSocket*>(QObject::sender());
    const auto clientID = m_socketList.key(socket);
    out << QObject::tr("WebSocket disconnected with clientID %1").arg(clientID) << Qt::endl;
    if (!clientID.isEmpty()) {
        m_socketList.remove(clientID);
    }
}

void WebProxy::webSocketServerError(QWebSocketProtocol::CloseCode closeCode)
{
    err << QObject::tr("WebSocket has an error: %1 %2")
               .arg(QString::number(closeCode), m_webSocketServer->errorString())
        << Qt::endl;
}

QJsonObject WebProxy::parseMessage(const QString& message)
{
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (json.isNull()) {
        err << QObject::tr("Failed to parse message as JSON: %1").arg(error.errorString()) << Qt::endl;
        return {};
    }
    return json.object();
}
