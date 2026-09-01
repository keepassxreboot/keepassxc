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

#include "BrowserWebSocketHost.h"
#include "BrowserShared.h"

#include <QJsonDocument>
#include <QtWebSockets/qwebsocket.h>
#include <QtWebSockets/qwebsocketserver.h>

#ifdef Q_OS_WIN
#include <fcntl.h>
#undef NOMINMAX
#define NOMINMAX
#include <windows.h>
#else
#include <sys/socket.h>
#endif

BrowserWebSocketHost::BrowserWebSocketHost(QObject* parent)
    : QObject(parent)
{
    m_webSocketServer =
        new QWebSocketServer(QStringLiteral("KeePassXC HTTP server"), QWebSocketServer::NonSecureMode, this);
}

BrowserWebSocketHost::~BrowserWebSocketHost()
{
    stop();
}

void BrowserWebSocketHost::start()
{
    if (!m_webSocketServer) {
        m_webSocketServer =
            new QWebSocketServer(QStringLiteral("KeePassXC HTTP server"), QWebSocketServer::NonSecureMode, this);
    }

    if (!m_webSocketServer->isListening()) {
        m_webSocketServer->listen(QHostAddress::LocalHost, 7580);
        connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &BrowserWebSocketHost::clientConnected);
        connect(m_webSocketServer, &QWebSocketServer::closed, this, [&] { m_webSocketServer->deleteLater(); });
    }
}

void BrowserWebSocketHost::stop()
{
    m_socketList.clear();
    m_webSocketServer->close();
}

void BrowserWebSocketHost::clientConnected()
{
    auto socket = m_webSocketServer->nextPendingConnection();
    if (socket) {
        m_socketList.append(socket);
        connect(socket, &QWebSocket::textMessageReceived, this, &BrowserWebSocketHost::readClientMessage);
        connect(socket, &QWebSocket::disconnected, this, &BrowserWebSocketHost::clientDisconnected);
    }
}

void BrowserWebSocketHost::readClientMessage(QString message)
{
    auto* socket = qobject_cast<QWebSocket*>(QObject::sender());
    if (!socket || !socket->isValid()) {
        return;
    }

    socket->setReadBufferSize(BrowserShared::NATIVEMSG_MAX_LENGTH);
    socket->setOutgoingFrameSize(BrowserShared::NATIVEMSG_MAX_LENGTH);

    QJsonParseError error;
    auto json = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (json.isNull()) {
        qWarning() << "Failed to read proxy message: " << error.errorString();
        return;
    }

    emit clientMessageReceived(socket, json.object());
}

void BrowserWebSocketHost::broadcastClientMessage(const QJsonObject& json)
{
    QString reply(QJsonDocument(json).toJson(QJsonDocument::Compact));
    for (const auto socket : m_socketList) {
        sendClientData(socket, reply);
    }
}

void BrowserWebSocketHost::sendClientMessage(QWebSocket* socket, const QJsonObject& json)
{
    QString reply(QJsonDocument(json).toJson(QJsonDocument::Compact));
    sendClientData(socket, reply);
}

void BrowserWebSocketHost::sendClientData(QWebSocket* socket, const QString& data)
{
    if (socket && socket->isValid() && socket->state() == QAbstractSocket::ConnectedState) {
        socket->sendTextMessage(data);
        socket->flush();
    }
}

void BrowserWebSocketHost::clientDisconnected()
{
    auto socket = qobject_cast<QWebSocket*>(QObject::sender());
    m_socketList.removeOne(socket);
}
