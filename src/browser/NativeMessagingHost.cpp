/*
*  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "NativeMessagingHost.h"
#include "BrowserSettings.h"
#include "sodium.h"
#include <QMutexLocker>
#include <QtNetwork>
#include <iostream>

#ifdef Q_OS_WIN
#include <Winsock2.h>
#endif

NativeMessagingHost::NativeMessagingHost(DatabaseTabWidget* parent, const bool enabled)
    : NativeMessagingBase(enabled)
    , m_mutex(QMutex::Recursive)
    , m_browserClients(m_browserService)
    , m_browserService(parent)
{
    m_localServer.reset(new QLocalServer(this));
    m_localServer->setSocketOptions(QLocalServer::UserAccessOption);
    m_running.store(false);

    if (BrowserSettings::isEnabled() && !m_running) {
        run();
    }

    connect(&m_browserService, SIGNAL(databaseLocked()), this, SLOT(databaseLocked()));
    connect(&m_browserService, SIGNAL(databaseUnlocked()), this, SLOT(databaseUnlocked()));
}

NativeMessagingHost::~NativeMessagingHost()
{
    stop();
}

int NativeMessagingHost::init()
{
    QMutexLocker locker(&m_mutex);
    return sodium_init();
}

void NativeMessagingHost::run()
{
    QMutexLocker locker(&m_mutex);
    if (!m_running.load() && init() == -1) {
        return;
    }

    // Update KeePassXC/keepassxc-proxy binary paths to Native Messaging scripts
    if (BrowserSettings::updateBinaryPath()) {
        BrowserSettings::updateBinaryPaths(BrowserSettings::useCustomProxy() ? BrowserSettings::customProxyLocation()
                                                                             : "");
    }

    m_running.store(true);
#ifdef Q_OS_WIN
    m_future =
        QtConcurrent::run(this, static_cast<void (NativeMessagingHost::*)()>(&NativeMessagingHost::readNativeMessages));
#endif

    if (BrowserSettings::supportBrowserProxy()) {
        QString serverPath = getLocalServerPath();
        QFile::remove(serverPath);

        // Ensure that STDIN is not being listened when proxy is used
        if (m_notifier && m_notifier->isEnabled()) {
            m_notifier->setEnabled(false);
        }

        if (m_localServer->isListening()) {
            m_localServer->close();
        }

        m_localServer->listen(serverPath);
        connect(m_localServer.data(), SIGNAL(newConnection()), this, SLOT(newLocalConnection()));
    } else {
        m_localServer->close();
    }
}

void NativeMessagingHost::stop()
{
    databaseLocked();
    QMutexLocker locker(&m_mutex);
    m_socketList.clear();
    m_running.testAndSetOrdered(true, false);
    m_future.waitForFinished();
    m_localServer->close();
}

void NativeMessagingHost::readLength()
{
    quint32 length = 0;
    std::cin.read(reinterpret_cast<char*>(&length), 4);
    if (!std::cin.eof() && length > 0) {
        readStdIn(length);
    } else {
        m_notifier->setEnabled(false);
    }
}

void NativeMessagingHost::readStdIn(const quint32 length)
{
    if (length <= 0) {
        return;
    }

    QByteArray arr;
    arr.reserve(length);

    QMutexLocker locker(&m_mutex);

    for (quint32 i = 0; i < length; ++i) {
        int c = std::getchar();
        if (c == EOF) {
            // message ended prematurely, ignore it and return
            return;
        }
        arr.append(static_cast<char>(c));
    }

    if (arr.length() > 0) {
        sendReply(m_browserClients.readResponse(arr));
    }
}

void NativeMessagingHost::newLocalConnection()
{
    QLocalSocket* socket = m_localServer->nextPendingConnection();
    if (socket) {
        connect(socket, SIGNAL(readyRead()), this, SLOT(newLocalMessage()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectSocket()));
    }
}

void NativeMessagingHost::newLocalMessage()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(QObject::sender());
    if (!socket || socket->bytesAvailable() <= 0) {
        return;
    }

    socket->setReadBufferSize(NATIVE_MSG_MAX_LENGTH);
    int socketDesc = socket->socketDescriptor();
    if (socketDesc) {
        int max = NATIVE_MSG_MAX_LENGTH;
        setsockopt(socketDesc, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&max), sizeof(max));
    }

    QByteArray arr = socket->readAll();
    if (arr.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    if (!m_socketList.contains(socket)) {
        m_socketList.push_back(socket);
    }

    QString reply = jsonToString(m_browserClients.readResponse(arr));
    if (socket && socket->isValid() && socket->state() == QLocalSocket::ConnectedState) {
        QByteArray arr = reply.toUtf8();
        socket->write(arr.constData(), arr.length());
        socket->flush();
    }
}

void NativeMessagingHost::sendReplyToAllClients(const QJsonObject& json)
{
    QString reply = jsonToString(json);
    QMutexLocker locker(&m_mutex);
    for (const auto socket : m_socketList) {
        if (socket && socket->isValid() && socket->state() == QLocalSocket::ConnectedState) {
            QByteArray arr = reply.toUtf8();
            socket->write(arr.constData(), arr.length());
            socket->flush();
        }
    }
}

void NativeMessagingHost::disconnectSocket()
{
    QLocalSocket* socket(qobject_cast<QLocalSocket*>(QObject::sender()));
    QMutexLocker locker(&m_mutex);
    for (auto s : m_socketList) {
        if (s == socket) {
            m_socketList.removeOne(s);
        }
    }
}

void NativeMessagingHost::removeSharedEncryptionKeys()
{
    QMutexLocker locker(&m_mutex);
    m_browserService.removeSharedEncryptionKeys();
}

void NativeMessagingHost::removeStoredPermissions()
{
    QMutexLocker locker(&m_mutex);
    m_browserService.removeStoredPermissions();
}

void NativeMessagingHost::databaseLocked()
{
    QJsonObject response;
    response["action"] = "database-locked";
    sendReplyToAllClients(response);
}

void NativeMessagingHost::databaseUnlocked()
{
    QJsonObject response;
    response["action"] = "database-unlocked";
    sendReplyToAllClients(response);
}
