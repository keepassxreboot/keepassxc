#include "private/qhttpclient_private.hpp"

#include <QTimerEvent>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
QHttpClient::QHttpClient(QObject *parent)
    : QObject(parent), d_ptr(new QHttpClientPrivate(this)) {
    QHTTP_LINE_LOG
}

QHttpClient::QHttpClient(QHttpClientPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd) {
    QHTTP_LINE_LOG
}

QHttpClient::~QHttpClient() {
    QHTTP_LINE_LOG
}

quint32
QHttpClient::timeOut() const {
    return d_func()->itimeOut;
}

void
QHttpClient::setTimeOut(quint32 t) {
    d_func()->itimeOut = t;
}

bool
QHttpClient::isOpen() const {
    return d_func()->isocket.isOpen();
}

void
QHttpClient::killConnection() {
    Q_D(QHttpClient);

    d->iconnectingTimer.stop();
    d->itimer.stop();
    d->isocket.close();
}

TBackend
QHttpClient::backendType() const {
    return d_func()->isocket.ibackendType;
}

QTcpSocket*
QHttpClient::tcpSocket() const {
    return d_func()->isocket.itcpSocket;
}

QLocalSocket*
QHttpClient::localSocket() const {
    return d_func()->isocket.ilocalSocket;
}

void
QHttpClient::setConnectingTimeOut(quint32 timeout) {
    Q_D(QHttpClient);

    if ( timeout == 0 ) {
        d->iconnectingTimer.stop();

    } else {
        d->iconnectingTimer.start(timeout,
                Qt::CoarseTimer,
                this
                );
    }
}

bool
QHttpClient::request(THttpMethod method, QUrl url,
                     const TRequstHandler &reqHandler,
                     const TResponseHandler &resHandler) {
    Q_D(QHttpClient);

    d->ireqHandler   = nullptr;
    d->irespHandler  = nullptr;

    // if url is a local file (UNIX socket) the host could be empty!
    if ( !url.isValid()    ||    url.isEmpty()    /*||    url.host().isEmpty()*/ )
        return false;

    // process handlers
    if ( resHandler ) {
        d->irespHandler = resHandler;

        if ( reqHandler )
            d->ireqHandler = reqHandler;
        else
            d->ireqHandler = [](QHttpRequest* req) ->void {
                req->addHeader("connection", "close");
                req->end();
            };
    }

    auto requestCreator = [this, method, url]() {
        // create request object
        if ( d_ptr->ilastRequest )
            d_ptr->ilastRequest->deleteLater();

        d_ptr->ilastRequest = new QHttpRequest(this);
        QObject::connect(d_ptr->ilastRequest, &QHttpRequest::done, [this](bool wasTheLastPacket){
            d_ptr->ikeepAlive = !wasTheLastPacket;
        });

        d_ptr->ilastRequest->d_ptr->imethod  = method;
        d_ptr->ilastRequest->d_ptr->iurl     = url;
    };

    // connecting to host/server must be the last thing. (after all function handlers and ...)
    // check for type
    if ( url.scheme().toLower() == QLatin1String("file") ) {
        d->isocket.ibackendType = ELocalSocket;
        d->initializeSocket();

        requestCreator();

        if ( d->isocket.isOpen() )
            d->onConnected();
        else
            d->isocket.connectTo(url);

    } else {
        d->isocket.ibackendType = ETcpSocket;
        d->initializeSocket();

        requestCreator();

        if ( d->isocket.isOpen() )
            d->onConnected();
        else
            d->isocket.connectTo(url.host(), url.port(80));
    }

    return true;
}

void
QHttpClient::timerEvent(QTimerEvent *e) {
    Q_D(QHttpClient);

    if ( e->timerId() == d->itimer.timerId() ) {
        killConnection();

    } else if ( e->timerId() == d->iconnectingTimer.timerId() ) {
        d->iconnectingTimer.stop();
        emit connectingTimeOut();
    }
}

void
QHttpClient::onRequestReady(QHttpRequest *req) {
    emit httpConnected(req);
}

void
QHttpClient::onResponseReady(QHttpResponse *res) {
    emit newResponse(res);
}

///////////////////////////////////////////////////////////////////////////////

// if server closes the connection, ends the response or by any other reason
// the socket disconnects, then the irequest and iresponse instances may have
// been deleted. In these situations reading more http body or emitting end()
// for incoming request are not possible:
// if ( ilastRequest == nullptr )
//     return 0;



int
QHttpClientPrivate::messageBegin(http_parser*) {
    itempHeaderField.clear();
    itempHeaderValue.clear();

    return 0;
}

int
QHttpClientPrivate::status(http_parser* parser, const char* at, size_t length) {
    if ( ilastResponse )
        ilastResponse->deleteLater();

    ilastResponse = new QHttpResponse(q_func());
    ilastResponse->d_func()->istatus  = static_cast<TStatusCode>(parser->status_code);
    ilastResponse->d_func()->iversion = QString("%1.%2")
                                        .arg(parser->http_major)
                                        .arg(parser->http_minor);
    ilastResponse->d_func()->icustomStatusMessage = QString::fromUtf8(at, length);

    return 0;
}

int
QHttpClientPrivate::headerField(http_parser*, const char* at, size_t length) {
    if ( ilastResponse == nullptr )
        return 0;

    // insert the header we parsed previously
    // into the header map
    if ( !itempHeaderField.isEmpty() && !itempHeaderValue.isEmpty() ) {
        // header names are always lower-cased
        ilastResponse->d_func()->iheaders.insert(
                    itempHeaderField.toLower(),
                    itempHeaderValue.toLower()
                    );
        // clear header value. this sets up a nice
        // feedback loop where the next time
        // HeaderValue is called, it can simply append
        itempHeaderField.clear();
        itempHeaderValue.clear();
    }

    itempHeaderField.append(at, length);
    return 0;
}

int
QHttpClientPrivate::headerValue(http_parser*, const char* at, size_t length) {

    itempHeaderValue.append(at, length);
    return 0;
}

int
QHttpClientPrivate::headersComplete(http_parser*) {
    if ( ilastResponse == nullptr )
        return 0;

    // Insert last remaining header
    ilastResponse->d_func()->iheaders.insert(
                itempHeaderField.toLower(),
                itempHeaderValue.toLower()
                );

    if ( irespHandler )
        irespHandler(ilastResponse);
    else
        q_func()->onResponseReady(ilastResponse);

    return 0;
}

int
QHttpClientPrivate::body(http_parser*, const char* at, size_t length) {
    if ( ilastResponse == nullptr )
        return 0;

    ilastResponse->d_func()->ireadState = QHttpResponsePrivate::EPartial;

    if ( ilastResponse->d_func()->icollectRequired ) {
        if ( !ilastResponse->d_func()->append(at, length) ) {
            // forcefully dispatch the ilastResponse
            finalizeConnection();
        }

        return 0;
    }

    emit ilastResponse->data(QByteArray(at, length));
    return 0;
}

int
QHttpClientPrivate::messageComplete(http_parser*) {
    if ( ilastResponse == nullptr )
        return 0;

    // response is done
    finalizeConnection();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
