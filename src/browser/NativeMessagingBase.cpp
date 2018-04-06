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

#include "NativeMessagingBase.h"
#include <QStandardPaths>

#if defined(Q_OS_UNIX) && !defined(Q_OS_LINUX)
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/epoll.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

NativeMessagingBase::NativeMessagingBase(const bool enabled)
{
#ifdef Q_OS_WIN
    Q_UNUSED(enabled);
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#else
    if (enabled) {
        m_notifier.reset(new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this));
        connect(m_notifier.data(), SIGNAL(activated(int)), this, SLOT(newNativeMessage()));
    }
#endif
}

void NativeMessagingBase::newNativeMessage()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_LINUX)
    struct kevent ev[1];
    struct timespec ts = {5, 0};

    int fd = kqueue();
    if (fd == -1) {
        m_notifier->setEnabled(false);
        return;
    }

    EV_SET(ev, fileno(stdin), EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(fd, ev, 1, nullptr, 0, &ts) == -1) {
        m_notifier->setEnabled(false);
        return;
    }

    int ret = kevent(fd, NULL, 0, ev, 1, &ts);
    if (ret < 1) {
        m_notifier->setEnabled(false);
        ::close(fd);
        return;
    }
#elif defined(Q_OS_LINUX)
    int fd = epoll_create(5);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    if (epoll_ctl(fd, EPOLL_CTL_ADD, 0, &event) != 0) {
        m_notifier->setEnabled(false);
        return;
    }

    if (epoll_wait(fd, &event, 1, 5000) < 1) {
        m_notifier->setEnabled(false);
        ::close(fd);
        return;
    }
#endif
    readLength();
#ifndef Q_OS_WIN
    ::close(fd);
#endif
}

void NativeMessagingBase::readNativeMessages()
{
#ifdef Q_OS_WIN
    quint32 length = 0;
    while (m_running.load() && !std::cin.eof()) {
        length = 0;
        std::cin.read(reinterpret_cast<char*>(&length), 4);
        readStdIn(length);
        QThread::msleep(1);
    }
#endif
}

QString NativeMessagingBase::jsonToString(const QJsonObject& json) const
{
    return QString(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void NativeMessagingBase::sendReply(const QJsonObject& json)
{
    if (!json.isEmpty()) {
        sendReply(jsonToString(json));
    }
}

void NativeMessagingBase::sendReply(const QString& reply)
{
    if (!reply.isEmpty()) {
        QByteArray bytes = reply.toUtf8();
        uint len = bytes.size();
        std::cout << char(((len >> 0) & 0xFF)) << char(((len >> 8) & 0xFF)) << char(((len >> 16) & 0xFF))
                  << char(((len >> 24) & 0xFF));
        std::cout << reply.toStdString() << std::flush;
    }
}

QString NativeMessagingBase::getLocalServerPath() const
{
    const QString serverPath = "/kpxc_server";
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Use XDG_RUNTIME_DIR instead of /tmp if it's available
    QString path = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return path.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::TempLocation) + serverPath : path + serverPath;
#else // Q_OS_MAC, Q_OS_WIN and others
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + serverPath;
#endif
}
