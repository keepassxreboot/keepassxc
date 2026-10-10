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

#include "BrowserHost.h"
#include "BrowserShared.h"

#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>

#ifdef Q_OS_WIN
#include <fcntl.h>
#undef NOMINMAX
#define NOMINMAX
#include <windows.h>
#else
#include <sys/socket.h>
#endif

BrowserHost::BrowserHost(QObject* parent)
    : QObject(parent)
{
    m_localServer = new QLocalServer(this);
    m_localServer->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_localServer.data(), SIGNAL(newConnection()), this, SLOT(proxyConnected()));
}

BrowserHost::~BrowserHost()
{
    stop();
}

void BrowserHost::start()
{
    if (!m_localServer->isListening()) {
        m_localServer->listen(BrowserShared::localServerPath());
    }
}

void BrowserHost::stop()
{
    m_socketList.clear();
    m_localServer->close();
}

void BrowserHost::proxyConnected()
{
    auto socket = m_localServer->nextPendingConnection();
    if (socket) {
        m_socketList.append(socket);
        connect(socket, SIGNAL(readyRead()), this, SLOT(readProxyMessage()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(proxyDisconnected()));
    }
}

void BrowserHost::readProxyMessage()
{
    auto* socket = qobject_cast<QLocalSocket*>(QObject::sender());
    if (!socket || socket->bytesAvailable() <= 0) {
        return;
    }

    socket->setReadBufferSize(BrowserShared::SOCKET_BUFFER_SIZE);
    int socketDesc = socket->socketDescriptor();
    if (socketDesc) {
        int max = BrowserShared::SOCKET_BUFFER_SIZE;
        setsockopt(socketDesc, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&max), sizeof(max));
    }

    const auto messages = parseSocketData(socket->readAll(), socketDesc);
    for (const auto& message : messages) {
        if (!message.isEmpty()) {
            emit clientMessageReceived(socket, message);
        }
    }
}

void BrowserHost::broadcastClientMessage(const QJsonObject& json)
{
    const auto reply(QJsonDocument(json).toJson(QJsonDocument::Compact));
    for (const auto socket : m_socketList) {
        sendClientData(socket, reply);
    }
}

void BrowserHost::sendClientMessage(QLocalSocket* socket, const QJsonObject& json)
{
    const auto reply(QJsonDocument(json).toJson(QJsonDocument::Compact));
    sendClientData(socket, reply);
}

void BrowserHost::sendClientData(QLocalSocket* socket, const QString& data)
{
    if (socket && socket->isValid() && socket->state() == QLocalSocket::ConnectedState) {
        const auto arr = data.toUtf8();
        socket->write(arr.constData(), arr.length());
        socket->flush();
    }
}

// Parses coalesced messages
QList<QJsonObject>
BrowserHost::parseSocketData(const QByteArray& socketData, const int socketDesc, const qsizetype maxLength)
{
    QList<QJsonObject> messages;

    // Split, and put } back to the messages
    auto splittedSocketData = socketData.split('}');
    for (auto i = 0; i < splittedSocketData.size() - 1; ++i) {
        splittedSocketData[i].append('}');
    }

    for (const auto& data : splittedSocketData) {
        if (data.length() > 0) {
            const auto parsedMessage = parseMessage(data, socketDesc, maxLength);
            if (!parsedMessage.isEmpty()) {
                messages << parsedMessage;
            }
        }
    }

    return messages;
}

// Parses fragmented messages, or messages with garbage data
// The JSON data recevied is always just one object with keys & values. No nested objects, arrays etc. are present.
// Coalesced messages rely entirely on that message format.
QJsonObject BrowserHost::parseMessage(const QByteArray& socketData, const int socketDesc, const qsizetype maxLength)
{
    if (socketData.isEmpty()) {
        return {};
    }

    // Ignore any garbage at the beginning and try to find the position where JSON data starts
    auto startPos = socketData.indexOf('{');
    if (startPos < 0) {
        //  Allow -1 because message can be fragmented. Set startPos back to 0.
        startPos = 0;
    }

    const auto message = socketData.mid(startPos);
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(message, &error);
    if (json.isNull()) {
        qWarning() << "Failed to read proxy message: " << error.error << error.errorString();

        // First fragmented message is of these errors. Partial message must be stored to a temporary buffer.
        if (error.error == QJsonParseError::UnterminatedObject || error.error == QJsonParseError::UnterminatedArray
            || error.error == QJsonParseError::UnterminatedString) {
            m_messageBuffer.insert(socketDesc, message);
            return {};
        }

        // Nth fragmented message can be an illegal value or illegal number
        if (error.error == QJsonParseError::IllegalValue || error.error == QJsonParseError::IllegalNumber) {
            return parseFragmentedMessage(message, socketDesc, maxLength);
        }

        // This error can happen with two different scenarios:
        // 1. Identifies a non-fragmented message with garbage at the end.
        // 2. Identifies a fragmented message and passes it forward.
        if (error.error == QJsonParseError::GarbageAtEnd && error.offset > 0) {
            // Parse message before the error offset and parse the JSON again
            const auto parsedMessage = message.left(error.offset);
            if (parsedMessage.startsWith('{')) {
                json = QJsonDocument::fromJson(parsedMessage, &error);
                if (!json.isNull() && error.error == QJsonParseError::NoError) {
                    m_messageBuffer.remove(socketDesc);
                    return json.object();
                }
            } else {
                return parseFragmentedMessage(message, socketDesc, maxLength);
            }
        }
    } else {
        m_messageBuffer.remove(socketDesc);
        return json.object();
    }

    return {};
}

// Returns the previous message(s) from the buffer, combines them with the current one, and parses again
QJsonObject
BrowserHost::parseFragmentedMessage(const QByteArray& message, const int socketDesc, const qsizetype maxLength)
{
    auto currentBuffer = m_messageBuffer.value(socketDesc);
    if (currentBuffer.length() == 0) {
        qWarning() << "Previous buffer not found.";
        m_messageBuffer.remove(socketDesc);
        return {};
    }

    if (currentBuffer.length() + message.length() > maxLength) {
        qWarning() << "Combined fragmented messages exceeded the maximum allowed length.";
        m_messageBuffer.remove(socketDesc);
        return {};
    }

    currentBuffer.append(message);
    return parseMessage(currentBuffer, socketDesc, maxLength);
}

void BrowserHost::proxyDisconnected()
{
    auto socket = qobject_cast<QLocalSocket*>(QObject::sender());
    m_socketList.removeOne(socket);
}
