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
    QByteArray m_socketBuffer;

    Q_DISABLE_COPY(NativeMessagingProxy)
};

/**
 * Take the complete messages out of the data read from the application. Replies can arrive
 * coalesced or fragmented. Like BrowserHost::parseSocketData, this relies on every message
 * being one flat JSON object. An incomplete message stays in the buffer.
 */
inline QList<QByteArray> takeSocketMessages(QByteArray& buffer)
{
    QList<QByteArray> messages;
    qsizetype end;
    while ((end = buffer.indexOf('}')) != -1) {
        messages << buffer.left(end + 1);
        buffer.remove(0, end + 1);
    }
    return messages;
}

#endif // NATIVEMESSAGINGPROXY_H
