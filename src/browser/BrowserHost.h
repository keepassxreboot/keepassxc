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

#ifndef KEEPASSXC_NATIVEMESSAGINGHOST_H
#define KEEPASSXC_NATIVEMESSAGINGHOST_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>

class QLocalServer;
class QLocalSocket;
class QString;

class BrowserHost : public QObject
{
    Q_OBJECT

public:
    explicit BrowserHost(QObject* parent = nullptr);
    ~BrowserHost() override;

    void start();
    void stop();

    void broadcastClientMessage(const QJsonObject& json);
    void sendClientMessage(QLocalSocket* socket, const QJsonObject& json);

signals:
    void clientMessageReceived(QLocalSocket* socket, const QJsonObject& json);

private slots:
    void proxyConnected();
    void readProxyMessage();
    void proxyDisconnected();

private:
    void sendClientData(QLocalSocket* socket, const QString& data);

private:
    QPointer<QLocalServer> m_localServer;
    QList<QLocalSocket*> m_socketList;
};

#endif // KEEPASSXC_NATIVEMESSAGINGHOST_H
