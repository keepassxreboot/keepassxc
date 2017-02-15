/** private imeplementation.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_CONNECTION_PRIVATE_HPP
#define QHTTPSERVER_CONNECTION_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////

#include "qhttpserverconnection.hpp"
#include "httpparser.hxx"
#include "qhttpserverrequest.hpp"
#include "qhttpserverresponse.hpp"

#include "private/qhttpserverrequest_private.hpp"
#include "private/qhttpserverresponse_private.hpp"

#include <QBasicTimer>
#include <QFile>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
class QHttpConnectionPrivate  :
    public details::HttpRequestParser<QHttpConnectionPrivate>
{
    Q_DECLARE_PUBLIC(QHttpConnection)

public:
    explicit QHttpConnectionPrivate(QHttpConnection* q) : q_ptr(q) {

        QObject::connect(
            q_func(), &QHttpConnection::disconnected,
            [this](){ release(); }
        );

        QHTTP_LINE_DEEPLOG
    }

    virtual ~QHttpConnectionPrivate() {
        QHTTP_LINE_DEEPLOG
    }

    void createSocket(qintptr sokDesc, TBackend bend) {
        isocket.ibackendType = bend;

        if ( bend == ETcpSocket ) {
            initTcpSocket(sokDesc);

        } else if ( bend == ELocalSocket ) {
            initLocalSocket(sokDesc);
        }
    }

    void release() {
        // if socket drops and http_parser can not call
        // messageComplete, dispatch the ilastRequest
        finalizeConnection();

        isocket.disconnectAllQtConnections();
        isocket.release();

        if ( ilastRequest ) {
            ilastRequest->deleteLater();
            ilastRequest  = nullptr;
        }

        if ( ilastResponse ) {
            ilastResponse->deleteLater();
            ilastResponse = nullptr;
        }

        q_func()->deleteLater();
    }

public:
    void onReadyRead() {
        while ( isocket.bytesAvailable() > 0 ) {
            char buffer[4097] = {0};
            size_t readLength = (size_t) isocket.readRaw(buffer, 4096);

            parse(buffer, readLength);
        }
    }

    void finalizeConnection() {
        if ( ilastRequest == nullptr )
            return;

        ilastRequest->d_func()->finalizeSending([this]{
            emit ilastRequest->end();
        });
    }

public:
    int  messageBegin(http_parser* parser);
    int  url(http_parser* parser, const char* at, size_t length);
    int  status(http_parser*, const char*, size_t) {
        return 0;   // not used in parsing incoming request.
    }
    int  headerField(http_parser* parser, const char* at, size_t length);
    int  headerValue(http_parser* parser, const char* at, size_t length);
    int  headersComplete(http_parser* parser);
    int  body(http_parser* parser, const char* at, size_t length);
    int  messageComplete(http_parser* parser);

private:
    void initTcpSocket(qintptr sokDesc) {
        QTcpSocket* sok    = new QTcpSocket( q_func() );
        isocket.itcpSocket = sok;
        sok->setSocketDescriptor(sokDesc);

        QObject::connect(
                sok,  &QTcpSocket::readyRead,
                [this](){ onReadyRead(); }
                );
        QObject::connect(
                sok,  &QTcpSocket::bytesWritten,
                [this](){
                    auto btw = isocket.itcpSocket->bytesToWrite();
                    if ( btw == 0  &&  ilastResponse )
                        emit ilastResponse->allBytesWritten();
                });
        QObject::connect(
                sok,      &QTcpSocket::disconnected,
                q_func(), &QHttpConnection::disconnected,
                Qt::QueuedConnection
                );
    }

    void initLocalSocket(qintptr sokDesc) {
        QLocalSocket* sok    = new QLocalSocket( q_func() );
        isocket.ilocalSocket = sok;
        sok->setSocketDescriptor(sokDesc);

        QObject::connect(
                sok, &QLocalSocket::readyRead,
                [this](){ onReadyRead(); }
                );
        QObject::connect(
                sok, &QLocalSocket::bytesWritten,
                [this](){
                    auto btw = isocket.ilocalSocket->bytesToWrite();
                    if ( btw == 0  &&  ilastResponse )
                        emit ilastResponse->allBytesWritten();
                });
        QObject::connect(
                sok,      &QLocalSocket::disconnected,
                q_func(), &QHttpConnection::disconnected,
                Qt::QueuedConnection
                );
    }

protected:
    QHttpConnection* const q_ptr;

    QByteArray             itempUrl;

    // Since there can only be one request/response pair per connection at any
    // time even with pipelining.
    QHttpRequest*          ilastRequest  = nullptr;
    QHttpResponse*         ilastResponse = nullptr;

    TServerHandler         ihandler      = nullptr;

};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPSERVER_CONNECTION_PRIVATE_HPP
