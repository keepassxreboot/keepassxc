#include "private/qhttpserverrequest_private.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
QHttpRequest::QHttpRequest(QHttpConnection *conn)
    : QHttpAbstractInput(conn), d_ptr(new QHttpRequestPrivate(conn, this)) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpRequest::QHttpRequest(QHttpRequestPrivate &dd, QHttpConnection *conn)
    : QHttpAbstractInput(conn), d_ptr(&dd) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpRequest::~QHttpRequest() {
    QHTTP_LINE_LOG
}

THttpMethod
QHttpRequest::method() const {
    return d_func()->imethod;
}

const QString
QHttpRequest::methodString() const {
    return http_method_str(static_cast<http_method>(d_func()->imethod));
}

const QUrl&
QHttpRequest::url() const {
    return d_func()->iurl;
}

const QString&
QHttpRequest::httpVersion() const {
    return d_func()->iversion;
}

const THeaderHash&
QHttpRequest::headers() const {
    return d_func()->iheaders;
}

const QString&
QHttpRequest::remoteAddress() const {
    return d_func()->iremoteAddress;
}

quint16
QHttpRequest::remotePort() const {
    return d_func()->iremotePort;
}

bool
QHttpRequest::isSuccessful() const {
    return d_func()->isuccessful;
}

void
QHttpRequest::collectData(int atMost) {
    d_func()->collectData(atMost);
}

const QByteArray&
QHttpRequest::collectedData() const {
    return d_func()->icollectedData;
}

QHttpConnection*
QHttpRequest::connection() const {
    return d_ptr->iconnection;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
