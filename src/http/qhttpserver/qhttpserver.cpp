/*
 * Copyright 2011 Nikhil Marathe <nsm.nikhil@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE. 
 */

#include "qhttpserver.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QVariant>
#include <QtCore/QDebug>

#include "qhttpconnection.h"

QHash<int, QString> STATUS_CODES;

QHttpServer::QHttpServer(QObject *parent)
    : QObject(parent)
    , m_tcpServer(0)
{
#define STATUS_CODE(num, reason) STATUS_CODES.insert(num, reason);
// {{{
  STATUS_CODE(100, "Continue")
  STATUS_CODE(101, "Switching Protocols")
  STATUS_CODE(102, "Processing")                 // RFC 2518) obsoleted by RFC 4918
  STATUS_CODE(200, "OK")
  STATUS_CODE(201, "Created")
  STATUS_CODE(202, "Accepted")
  STATUS_CODE(203, "Non-Authoritative Information")
  STATUS_CODE(204, "No Content")
  STATUS_CODE(205, "Reset Content")
  STATUS_CODE(206, "Partial Content")
  STATUS_CODE(207, "Multi-Status")               // RFC 4918
  STATUS_CODE(300, "Multiple Choices")
  STATUS_CODE(301, "Moved Permanently")
  STATUS_CODE(302, "Moved Temporarily")
  STATUS_CODE(303, "See Other")
  STATUS_CODE(304, "Not Modified")
  STATUS_CODE(305, "Use Proxy")
  STATUS_CODE(307, "Temporary Redirect")
  STATUS_CODE(400, "Bad Request")
  STATUS_CODE(401, "Unauthorized")
  STATUS_CODE(402, "Payment Required")
  STATUS_CODE(403, "Forbidden")
  STATUS_CODE(404, "Not Found")
  STATUS_CODE(405, "Method Not Allowed")
  STATUS_CODE(406, "Not Acceptable")
  STATUS_CODE(407, "Proxy Authentication Required")
  STATUS_CODE(408, "Request Time-out")
  STATUS_CODE(409, "Conflict")
  STATUS_CODE(410, "Gone")
  STATUS_CODE(411, "Length Required")
  STATUS_CODE(412, "Precondition Failed")
  STATUS_CODE(413, "Request Entity Too Large")
  STATUS_CODE(414, "Request-URI Too Large")
  STATUS_CODE(415, "Unsupported Media Type")
  STATUS_CODE(416, "Requested Range Not Satisfiable")
  STATUS_CODE(417, "Expectation Failed")
  STATUS_CODE(418, "I\"m a teapot")              // RFC 2324
  STATUS_CODE(422, "Unprocessable Entity")       // RFC 4918
  STATUS_CODE(423, "Locked")                     // RFC 4918
  STATUS_CODE(424, "Failed Dependency")          // RFC 4918
  STATUS_CODE(425, "Unordered Collection")       // RFC 4918
  STATUS_CODE(426, "Upgrade Required")           // RFC 2817
  STATUS_CODE(500, "Internal Server Error")
  STATUS_CODE(501, "Not Implemented")
  STATUS_CODE(502, "Bad Gateway")
  STATUS_CODE(503, "Service Unavailable")
  STATUS_CODE(504, "Gateway Time-out")
  STATUS_CODE(505, "HTTP Version not supported")
  STATUS_CODE(506, "Variant Also Negotiates")    // RFC 2295
  STATUS_CODE(507, "Insufficient Storage")       // RFC 4918
  STATUS_CODE(509, "Bandwidth Limit Exceeded")
  STATUS_CODE(510, "Not Extended")                // RFC 2774
// }}}
}

QHttpServer::~QHttpServer()
{
}

void QHttpServer::newConnection()
{
    Q_ASSERT(m_tcpServer);
    while(m_tcpServer->hasPendingConnections()) {
        QHttpConnection *connection = new QHttpConnection(m_tcpServer->nextPendingConnection(), this);
        connect(connection, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
                this, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)));
    }
}

bool QHttpServer::listen(const QHostAddress &address, quint16 port)
{
    m_tcpServer = new QTcpServer;

    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    return m_tcpServer->listen(address, port);
}

bool QHttpServer::listen(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void QHttpServer::close()
{
  m_tcpServer->close();
}
