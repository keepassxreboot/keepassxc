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

#ifndef KEEPASSXC_BROWSERWEBSOCKETHOST_H
#define KEEPASSXC_BROWSERWEBSOCKETHOST_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>

class QWebSocketServer;
class QWebSocket;
class QString;

class BrowserWebSocketHost : public QObject
{
    Q_OBJECT

public:
    explicit BrowserWebSocketHost(QObject* parent = nullptr);
    ~BrowserWebSocketHost() override;

    void start();
    void stop();

    void broadcastClientMessage(const QJsonObject& json);
    void sendClientMessage(QWebSocket* socket, const QJsonObject& json);

signals:
    void clientMessageReceived(QWebSocket* socket, const QJsonObject& json);

private slots:
    void clientConnected();
    void readClientMessage(QString message);
    void clientDisconnected();

private:
    void sendClientData(QWebSocket* socket, const QString& data);

private:
    QPointer<QWebSocketServer> m_webSocketServer;
    QList<QWebSocket*> m_socketList;
};

#endif // KEEPASSXC_BROWSERWEBSOCKETHOST_H
