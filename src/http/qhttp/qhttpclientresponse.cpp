#include "private/qhttpclientresponse_private.hpp"
#include "qhttpclient.hpp"
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
QHttpResponse::QHttpResponse(QHttpClient *cli)
    : QHttpAbstractInput(cli), d_ptr(new QHttpResponsePrivate(cli, this)) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpResponse::QHttpResponse(QHttpResponsePrivate &dd, QHttpClient *cli)
    : QHttpAbstractInput(cli), d_ptr(&dd) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpResponse::~QHttpResponse() {
    QHTTP_LINE_LOG
}

TStatusCode
QHttpResponse::status() const {
    return d_func()->istatus;
}

const QString&
QHttpResponse::statusString() const {
    return d_func()->icustomStatusMessage;
}

const QString&
QHttpResponse::httpVersion() const {
    return d_func()->iversion;
}

const THeaderHash&
QHttpResponse::headers() const {
    return d_func()->iheaders;
}

bool
QHttpResponse::isSuccessful() const {
    return d_func()->isuccessful;
}

void
QHttpResponse::collectData(int atMost) {
    d_func()->collectData(atMost);
}

const QByteArray&
QHttpResponse::collectedData() const {
    return d_func()->icollectedData;
}

QHttpClient*
QHttpResponse::connection() const {
    return d_func()->iclient;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
