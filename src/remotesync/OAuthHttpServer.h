/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_OAUTHHTTPSERVER_H
#define KEEPASSXC_OAUTHHTTPSERVER_H

#include <QObject>
#include <QTcpServer>

class QTcpSocket;

class OAuthHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit OAuthHttpServer(QObject* parent = nullptr);
    ~OAuthHttpServer() override;

    // Start listening. port=0 means OS-assigned.
    // Returns true if server started successfully.
    bool start(quint16 port = 0);

    // The actual port the server is listening on (after start).
    quint16 port() const;

    // Stop listening and clean up connected sockets.
    void stop();

    // Whether the server is currently listening.
    bool isListening() const;

    // Set expected OAuth state parameter for CSRF validation.
    // If set, the callback must include a matching state= query parameter.
    void setExpectedState(const QString& state);

signals:
    void authCodeReceived(const QString& code);
    void authError(const QString& error);

private slots:
    void handleNewConnection();

private:
    void processRequest(QTcpSocket* socket);

    static constexpr int MaxRequestSize = 8192; // 8KB limit for HTTP request headers
    static constexpr int SocketTimeoutMs = 10000; // 10s per-socket read timeout

    QTcpServer* m_server = nullptr;
    bool m_codeReceived = false; // Prevents double-processing
    QString m_expectedState;
};

#endif // KEEPASSXC_OAUTHHTTPSERVER_H
