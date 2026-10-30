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

#include <istream>

class NativeMessagingProxy : public QObject
{
    Q_OBJECT
public:
    NativeMessagingProxy();
    ~NativeMessagingProxy() override = default;

signals:
    void stdinMessage(QByteArray message);

public slots:
    void transferSocketMessage();
    void transferStdinMessage(const QByteArray& message);
    void socketDisconnected();

private:
    void setupStandardInput();
    void setupLocalSocket();

private:
    QScopedPointer<QLocalSocket> m_localSocket;

    Q_DISABLE_COPY(NativeMessagingProxy)
};

/**
 * Read one native messaging frame: a 32-bit length in native byte order, then that many bytes.
 * The bytes are returned unchanged. A frame longer than maxLength is skipped and gives an
 * empty message.
 *
 * @return false when the input ends, including in the middle of a frame
 */
inline bool readNativeMessage(std::istream& in, QByteArray& message, quint32 maxLength)
{
    quint32 length = 0;
    if (!in.read(reinterpret_cast<char*>(&length), sizeof(length))) {
        return false;
    }

    if (length > maxLength) {
        message.clear();
        return static_cast<bool>(in.ignore(length));
    }

    message.resize(length);
    return static_cast<bool>(in.read(message.data(), length));
}

#endif // NATIVEMESSAGINGPROXY_H
