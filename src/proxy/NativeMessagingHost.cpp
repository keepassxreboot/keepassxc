/*
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

#include <QCoreApplication>
#include "NativeMessagingHost.h"

NativeMessagingHost::NativeMessagingHost() : NativeMessagingBase()
{
    m_localSocket = new QLocalSocket();
    m_localSocket->connectToServer(getLocalServerPath());
#ifdef Q_OS_WIN
    m_running.store(true);
    m_future = QtConcurrent::run(this, static_cast<void(NativeMessagingHost::*)()>(&NativeMessagingHost::readNativeMessages));
#endif
    connect(m_localSocket, SIGNAL(readyRead()), this, SLOT(newLocalMessage()));
    connect(m_localSocket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
    connect(m_localSocket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketStateChanged(QLocalSocket::LocalSocketState)));
}

NativeMessagingHost::~NativeMessagingHost()
{
#ifdef Q_OS_WIN
    m_future.waitForFinished();
#endif
}

void NativeMessagingHost::readLength()
{
    quint32 length = 0;
    std::cin.read(reinterpret_cast<char*>(&length), 4);
    if (!std::cin.eof() && length > 0) {
        readStdIn(length);
    } else {
    	QCoreApplication::quit();
    }
}

void NativeMessagingHost::readStdIn(const quint32 length)
{
    if (length <= 0) {
        return;
    }

    QByteArray arr;
    arr.reserve(length);

    for (quint32 i = 0; i < length; ++i) {
        int c = std::getchar();
        if (c == EOF) {
            // message ended prematurely, ignore it and return
            return;
        }
        arr.append(static_cast<char>(c));
    }

    if (arr.length() > 0 && m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState) {
        m_localSocket->write(arr.constData(), arr.length());
        m_localSocket->flush();
    }
}

void NativeMessagingHost::newLocalMessage()
{
    if (!m_localSocket || m_localSocket->bytesAvailable() <= 0) {
        return;
    }

    QByteArray arr = m_localSocket->readAll();
    if (!arr.isEmpty()) {
       sendReply(arr);
    }
}

void NativeMessagingHost::deleteSocket()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
    }
    m_localSocket->deleteLater();
    QCoreApplication::quit();
}

void NativeMessagingHost::socketStateChanged(QLocalSocket::LocalSocketState socketState)
{
    if (socketState == QLocalSocket::UnconnectedState || socketState == QLocalSocket::ClosingState) {
        m_running.testAndSetOrdered(true, false);
    }
}
