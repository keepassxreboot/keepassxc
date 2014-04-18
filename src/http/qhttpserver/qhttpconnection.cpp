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

#include "qhttpconnection.h"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtCore/QDebug>

#include "qhttprequest.h"
#include "qhttpresponse.h"

QHttpConnection::QHttpConnection(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_parser(0)
    , m_request(0)
{
    qDebug() << "Got new connection" << socket->peerAddress() << socket->peerPort();

    m_parser = (http_parser*)malloc(sizeof(http_parser));
    http_parser_init(m_parser, HTTP_REQUEST);

    m_parserSettings.on_message_begin = MessageBegin;
    m_parserSettings.on_url = Url;
    m_parserSettings.on_header_field = HeaderField;
    m_parserSettings.on_header_value = HeaderValue;
    m_parserSettings.on_headers_complete = HeadersComplete;
    m_parserSettings.on_body = Body;
    m_parserSettings.on_message_complete = MessageComplete;

    m_parser->data = this;

    connect(socket, SIGNAL(readyRead()), this, SLOT(parseRequest()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
}

QHttpConnection::~QHttpConnection()
{
    delete m_socket;
    m_socket = 0;

    free(m_parser);
    m_parser = 0;
}

void QHttpConnection::socketDisconnected()
{
    if(m_request) {
        if(m_request->successful()) {
          return;
        }
        m_request->setSuccessful(false);
        Q_EMIT m_request->end();
    }

    deleteLater();
}

void QHttpConnection::parseRequest()
{
    Q_ASSERT(m_parser);

    while(m_socket->bytesAvailable())
    {
        QByteArray arr = m_socket->readAll();
        http_parser_execute(m_parser, &m_parserSettings, arr.constData(), arr.size());
    }
}

void QHttpConnection::write(const QByteArray &data)
{
    m_socket->write(data);
}

void QHttpConnection::flush()
{
    m_socket->flush();
}

/********************
 * Static Callbacks *
 *******************/
int QHttpConnection::MessageBegin(http_parser *parser)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    theConnection->m_currentHeaders.clear();
    theConnection->m_request = new QHttpRequest(theConnection);
    return 0;
}

int QHttpConnection::HeadersComplete(http_parser *parser)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    /** set method **/
    theConnection->m_request->setMethod(static_cast<QHttpRequest::HttpMethod>(parser->method));

    /** set version **/
    theConnection->m_request->setVersion(QString("%1.%2").arg(parser->http_major).arg(parser->http_minor));

    // Insert last remaining header
    theConnection->m_currentHeaders[theConnection->m_currentHeaderField.toLower()] = theConnection->m_currentHeaderValue;
    theConnection->m_request->setHeaders(theConnection->m_currentHeaders);

    /** set client information **/
    theConnection->m_request->m_remoteAddress = theConnection->m_socket->peerAddress().toString();
    theConnection->m_request->m_remotePort = theConnection->m_socket->peerPort();

    QHttpResponse *response = new QHttpResponse(theConnection);
    if( parser->http_major < 1 || parser->http_minor < 1 )
        response->m_keepAlive = false;

    connect(theConnection, SIGNAL(destroyed()), response, SLOT(connectionClosed()));

    // we are good to go!
    Q_EMIT theConnection->newRequest(theConnection->m_request, response);
    return 0;
}

int QHttpConnection::MessageComplete(http_parser *parser)
{
    // TODO: do cleanup and prepare for next request
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    theConnection->m_request->setSuccessful(true);
    Q_EMIT theConnection->m_request->end();
    return 0;
}

int QHttpConnection::Url(http_parser *parser, const char *at, size_t length)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    QString url = QString::fromAscii(at, length);
    theConnection->m_request->setUrl(QUrl(url));
    return 0;
}

int QHttpConnection::HeaderField(http_parser *parser, const char *at, size_t length)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    // insert the header we parsed previously
    // into the header map
    if( !theConnection->m_currentHeaderField.isEmpty() && !theConnection->m_currentHeaderValue.isEmpty() )
    {
        // header names are always lower-cased
        theConnection->m_currentHeaders[theConnection->m_currentHeaderField.toLower()] = theConnection->m_currentHeaderValue;
        // clear header value. this sets up a nice
        // feedback loop where the next time
        // HeaderValue is called, it can simply append
        theConnection->m_currentHeaderField = QString();
        theConnection->m_currentHeaderValue = QString();
    }

    QString fieldSuffix = QString::fromAscii(at, length);
    theConnection->m_currentHeaderField += fieldSuffix;
    return 0;
}

int QHttpConnection::HeaderValue(http_parser *parser, const char *at, size_t length)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    QString valueSuffix = QString::fromAscii(at, length);
    theConnection->m_currentHeaderValue += valueSuffix;
    return 0;
}

int QHttpConnection::Body(http_parser *parser, const char *at, size_t length)
{
    QHttpConnection *theConnection = (QHttpConnection *)parser->data;
    Q_ASSERT(theConnection->m_request);

    Q_EMIT theConnection->m_request->data(QByteArray(at, length));
    return 0;
}
