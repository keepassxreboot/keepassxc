/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "NativeMessagingProxy.h"
#include "browser/BrowserShared.h"

#include <QCoreApplication>
#include <QFuture>
#include <QtConcurrent/qtconcurrentrun.h>

#include <iostream>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

NativeMessagingProxy::NativeMessagingProxy()
    : QObject()
{
    connect(this,
            &NativeMessagingProxy::stdinMessage,
            this,
            &NativeMessagingProxy::transferStdinMessage,
            Qt::QueuedConnection);

    setupStandardInput();
    setupLocalSocket();
}

void NativeMessagingProxy::setupStandardInput()
{
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#else
    setmode(fileno(stdin), _O_BINARY);
    setmode(fileno(stdout), _O_BINARY);
#endif
#endif

    // Forward the messages unchanged, the application parses them. It drops messages
    // longer than its socket buffer, so don't send those at all.
    auto res = QtConcurrent::run([this] {
        QByteArray message;
        while (readNativeMessage(std::cin, message, BrowserShared::SOCKET_BUFFER_SIZE)) {
            if (!message.isEmpty()) {
                emit stdinMessage(message);
            }
        }
        QCoreApplication::quit();
    });
}

void NativeMessagingProxy::transferStdinMessage(const QByteArray& message)
{
    if (m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState) {
        m_localSocket->write(message);
        m_localSocket->flush();
    }
}

void NativeMessagingProxy::setupLocalSocket()
{
    m_localSocket.reset(new QLocalSocket());
    m_localSocket->connectToServer(BrowserShared::localServerPath());
    m_localSocket->setReadBufferSize(BrowserShared::SOCKET_BUFFER_SIZE);
    int socketDesc = m_localSocket->socketDescriptor();
    if (socketDesc) {
        int max = BrowserShared::SOCKET_BUFFER_SIZE;
        setsockopt(socketDesc, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&max), sizeof(max));
    }

    connect(m_localSocket.data(), SIGNAL(readyRead()), this, SLOT(transferSocketMessage()));
    connect(m_localSocket.data(), SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
}

void NativeMessagingProxy::transferSocketMessage()
{
    auto msg = m_localSocket->readAll();
    if (!msg.isEmpty()) {
        // Explicitly write the message length as 1 byte chunks
        uint len = msg.size();
        std::cout.write(reinterpret_cast<char*>(&len), sizeof(len));

        // Write the message and flush the stream
        std::cout << msg.toStdString() << std::flush;
    }
}

void NativeMessagingProxy::socketDisconnected()
{
    // Shutdown the proxy when disconnected from the application
    QCoreApplication::quit();
}
