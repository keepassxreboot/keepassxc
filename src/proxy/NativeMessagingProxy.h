/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef NATIVEMESSAGINGPROXY_H
#define NATIVEMESSAGINGPROXY_H

#include <QLocalSocket>
#include <QObject>
#include <QScopedPointer>

class QWinEventNotifier;
class QSocketNotifier;

class NativeMessagingProxy : public QObject
{
    Q_OBJECT
public:
    NativeMessagingProxy();
    ~NativeMessagingProxy() override = default;

signals:
    void stdinMessage(QString msg);

public slots:
    void transferSocketMessage();
    void transferStdinMessage(const QString& msg);
    void socketDisconnected();

private:
    void setupStandardInput();
    void setupLocalSocket();

private:
    QScopedPointer<QLocalSocket> m_localSocket;

    Q_DISABLE_COPY(NativeMessagingProxy)
};

#endif // NATIVEMESSAGINGPROXY_H
