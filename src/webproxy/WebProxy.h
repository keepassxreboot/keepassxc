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

#ifndef KEEPASSXC_WEBPROXY_H
#define KEEPASSXC_WEBPROXY_H

#include <QHash>
#include <QJsonObject>
#include <QLocalSocket>
#include <QObject>
#include <QScopedPointer>
#include <QtWebSockets/qwebsocket.h>
#include <QtWebSockets/qwebsocketprotocol.h>
#include <QtWebSockets/qwebsocketserver.h>

class WebProxy : public QObject
{
    Q_OBJECT

public:
    explicit WebProxy(const QString& webSocketPort, const QString& customExtensionId, QObject* parent = nullptr);
    ~WebProxy() override;

signals:
    void webSocketMessageReceived(QWebSocket* socket, const QJsonObject& json);
    void webSocketMessage(QString msg);

private slots:
    void readLocalSocketMessage();
    void sendToLocalSocket(QWebSocket* socket, const QJsonObject& json);
    void localSocketConnected();
    void localSocketDisconnected();
    void localSocketErrored(QLocalSocket::LocalSocketError socketError);
    void readWebSocketMessage(QString message);
    void webSocketConnected();
    void webSocketDisconnected();
    void webSocketServerError(QWebSocketProtocol::CloseCode closeCode);
    void originAuthenticationRequired(QWebSocketCorsAuthenticator* authenticator);

private:
    void setupWebSocketServer(const QString& webSocketPort);
    void stopWebSocket();
    void setupLocalSocket();
    void sendToWebSocket(QWebSocket* socket, const QString& data);
    QJsonObject parseMessage(const QString& message);

private:
    QScopedPointer<QWebSocketServer> m_webSocketServer;
    QScopedPointer<QLocalSocket> m_localSocket;
    QHash<QString, QWebSocket*> m_socketList;
    QString m_customExtensionId;
};

#endif // KEEPASSXC_WEBPROXY_H
