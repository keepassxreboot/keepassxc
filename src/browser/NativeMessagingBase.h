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

#ifndef NATIVEMESSAGINGBASE_H
#define NATIVEMESSAGINGBASE_H

#include <QAtomicInteger>
#include <QFuture>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMutex>
#include <QObject>
#include <QSocketNotifier>
#include <QtConcurrent/QtConcurrent>
#include <iostream>
#include <unistd.h>

#ifndef Q_OS_WIN
#include <sys/types.h>
#include <sys/socket.h> 
#endif

static const int NATIVE_MSG_MAX_LENGTH = 1024*1024;

class NativeMessagingBase : public QObject
{
    Q_OBJECT

public:
    explicit NativeMessagingBase(const bool enabled);
    ~NativeMessagingBase() = default;

protected slots:
    void newNativeMessage();

protected:
    virtual void readLength() = 0;
    virtual void readStdIn(const quint32 length) = 0;
    void readNativeMessages();
    QString jsonToString(const QJsonObject& json) const;
    void sendReply(const QJsonObject& json);
    void sendReply(const QString& reply);
    QString getLocalServerPath() const;

protected:
    QAtomicInteger<quint8> m_running;
    QSharedPointer<QSocketNotifier> m_notifier;
    QFuture<void> m_future;
};

#endif // NATIVEMESSAGINGBASE_H
