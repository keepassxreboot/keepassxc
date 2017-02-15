/** @file qsocket.hpp
 *
 * @copyright (C) 2016
 * @date 2016.05.26
 * @version 1.0.0
 * @author amir zamani <azadkuh@live.com>
 *
 */

#ifndef __QHTTP_SOCKET_HPP__
#define __QHTTP_SOCKET_HPP__

#include "qhttpfwd.hpp"

#include <QTcpSocket>
#include <QLocalSocket>
#include <QUrl>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace details {
///////////////////////////////////////////////////////////////////////////////

/** an adapter for different socket types.
 * the main purpose of QHttp was to create a small HTTP server with ability to
 * support UNIX sockets (QLocalSocket)
 */
class QSocket
{
public:
    void close() {
        if ( itcpSocket )
            itcpSocket->close();

        if ( ilocalSocket )
            ilocalSocket->close();
    }

    void release() {
        close();
        if ( itcpSocket )
            itcpSocket->deleteLater();

        if ( ilocalSocket )
            ilocalSocket->deleteLater();

        itcpSocket   = nullptr;
        ilocalSocket = nullptr;
    }

    void flush() {
        if ( itcpSocket )
            itcpSocket->flush();

        else if ( ilocalSocket )
            ilocalSocket->flush();
    }

    bool isOpen() const {
        if ( ibackendType == ETcpSocket    &&    itcpSocket )
            return itcpSocket->isOpen()
                && itcpSocket->state() == QTcpSocket::ConnectedState;

        else if ( ibackendType == ELocalSocket    &&    ilocalSocket )
            return ilocalSocket->isOpen()
                && ilocalSocket->state() == QLocalSocket::ConnectedState;

        return false;
    }

    void connectTo(const QUrl& url) {
        if ( ilocalSocket )
            ilocalSocket->connectToServer(url.path());
    }

    void connectTo(const QString& host, quint16 port) {
        if ( itcpSocket )
            itcpSocket->connectToHost(host, port);
    }

    qint64 readRaw(char* buffer, int maxlen) {
        if ( itcpSocket )
            return itcpSocket->read(buffer, maxlen);

        else if ( ilocalSocket )
            return ilocalSocket->read(buffer, maxlen);

        return 0;
    }

    void writeRaw(const QByteArray& data) {
        if ( itcpSocket )
            itcpSocket->write(data);

        else if ( ilocalSocket )
            ilocalSocket->write(data);
    }

    qint64 bytesAvailable() {
        if ( itcpSocket )
            return itcpSocket->bytesAvailable();

        else if ( ilocalSocket )
            return ilocalSocket->bytesAvailable();

        return 0;
    }

    void disconnectAllQtConnections() {
        if ( itcpSocket )
            QObject::disconnect(itcpSocket, 0, 0, 0);

        if ( ilocalSocket )
            QObject::disconnect(ilocalSocket, 0, 0, 0);
    }

public:
    TBackend      ibackendType = ETcpSocket;
    QTcpSocket*   itcpSocket   = nullptr;
    QLocalSocket* ilocalSocket = nullptr;
}; // class QSocket

///////////////////////////////////////////////////////////////////////////////
} // namespace details
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTP_SOCKET_HPP__
