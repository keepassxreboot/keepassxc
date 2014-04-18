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

#include "qhttpresponse.h"

#include <QtCore/QDateTime>

#include "qhttpserver.h"
#include "qhttpconnection.h"

QHttpResponse::QHttpResponse(QHttpConnection *connection)
    // TODO: parent child relation
    : QObject(0)
    , m_connection(connection)
    , m_headerWritten(false)
    , m_sentConnectionHeader(false)
    , m_sentContentLengthHeader(false)
    , m_sentTransferEncodingHeader(false)
    , m_sentDate(false)
    , m_keepAlive(true)
    , m_last(false)
    , m_useChunkedEncoding(false)
    , m_finished(false)
{
}

QHttpResponse::~QHttpResponse()
{
}

void QHttpResponse::setHeader(const QString &field, const QString &value)
{
    if(m_finished) {
      return;
    }

    m_headers[field] = value;
}

void QHttpResponse::writeHeader(const char *field, const QString &value)
{
    if(m_finished) {
      return;
    }

    m_connection->write(field);
    m_connection->write(": ");
    m_connection->write(value.toUtf8());
    m_connection->write("\r\n");
}

void QHttpResponse::writeHeaders()
{
    if(m_finished) {
      return;
    }

    Q_FOREACH(QString name, m_headers.keys())
    {
        QString value = m_headers[name];
        if( name.compare("connection", Qt::CaseInsensitive) == 0 ) 
        {
            m_sentConnectionHeader = true;
            if( value == "close" )
                m_last = true;
            else
                m_keepAlive = true;
        }
        else if( name.compare("transfer-encoding", Qt::CaseInsensitive) == 0 )
        {
            m_sentTransferEncodingHeader = true;
            if( value == "chunked" )
                m_useChunkedEncoding = true;
        }
        else if( name.compare("content-length", Qt::CaseInsensitive) == 0 )
        {
            m_sentContentLengthHeader = true;
        }
        else if( name.compare("date", Qt::CaseInsensitive) == 0 )
        {
            m_sentDate = true;
        }
        //TODO: Expect case

        writeHeader(name.toAscii(), value.toAscii());
    }

    if( !m_sentConnectionHeader )
    {
        if( m_keepAlive &&
                ( m_sentContentLengthHeader || m_useChunkedEncoding ) )
        {
            writeHeader("Connection", "keep-alive");
        }
        else
        {
            m_last = true;
            writeHeader("Connection", "close");
        }
    }

    if( !m_sentContentLengthHeader && !m_sentTransferEncodingHeader )
    {
        if( m_useChunkedEncoding )
            writeHeader("Transfer-Encoding", "chunked");
        else
            m_last = true;
    }

    if( !m_sentDate )
    {
        writeHeader("Date", QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy hh:mm:ss G'M'T"));
    }
}

void QHttpResponse::writeHead(int status)
{
    if(m_finished) {
      return;
    }

    if( m_headerWritten ) return;

    m_connection->write(QString("HTTP/1.1 %1 %2\r\n").arg(status).arg(STATUS_CODES[status]).toAscii());
    
    writeHeaders();

    m_connection->write("\r\n");
    m_headerWritten = true;
}

void QHttpResponse::write(const QByteArray &data)
{
    if(m_finished) {
      return;
    }

    if( !m_headerWritten )
    {
        qDebug() << "You MUST call writeHead() before writing body data";
        return;
    }

    m_connection->write(data);
}

void QHttpResponse::write(const QString &data)
{
    if(m_finished) {
      return;
    }

    m_connection->write(data.toUtf8());
}

void QHttpResponse::end(const QString &data)
{
    if(m_finished) {
      return;
    }
    m_finished = true;

    write(data);

    Q_EMIT done();
    deleteLater();
    // TODO: end connection and delete ourselves
}

void QHttpResponse::connectionClosed()
{
  m_finished = true;
  deleteLater();
}
