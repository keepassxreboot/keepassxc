#include "private/qhttpserverresponse_private.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
QHttpResponse::QHttpResponse(QHttpConnection* conn)
    : QHttpAbstractOutput(conn) , d_ptr(new QHttpResponsePrivate(conn, this)) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpResponse::QHttpResponse(QHttpResponsePrivate& dd, QHttpConnection* conn)
    : QHttpAbstractOutput(conn) , d_ptr(&dd) {
    d_ptr->initialize();
    QHTTP_LINE_LOG
}

QHttpResponse::~QHttpResponse() {
    QHTTP_LINE_LOG
}

void
QHttpResponse::setStatusCode(TStatusCode code) {
    d_func()->istatus   = code;
}

void
QHttpResponse::setVersion(const QString &versionString) {
    d_func()->iversion  = versionString;
}

void
QHttpResponse::addHeader(const QByteArray &field, const QByteArray &value) {
    d_func()->addHeader(field, value);
}

THeaderHash&
QHttpResponse::headers() {
    return d_func()->iheaders;
}

void
QHttpResponse::write(const QByteArray &data) {
    d_func()->writeData(data);
}

void
QHttpResponse::end(const QByteArray &data) {
    Q_D(QHttpResponse);

    if ( d->endPacket(data) )
        emit done(!d->ikeepAlive);
}

QHttpConnection*
QHttpResponse::connection() const {
    return d_func()->iconnection;
}

///////////////////////////////////////////////////////////////////////////////
QByteArray
QHttpResponsePrivate::makeTitle() {

    QString title = QString("HTTP/%1 %2 %3\r\n")
                    .arg(iversion)
                    .arg(istatus)
                    .arg(Stringify::toString(istatus));

    return title.toLatin1();
}

void
QHttpResponsePrivate::prepareHeadersToWrite() {

    if ( !iheaders.contains("date") ) {
        // Sun, 06 Nov 1994 08:49:37 GMT - RFC 822. Use QLocale::c() so english is used for month and
        // day.
        QString dateString = QLocale::c().
            toString(QDateTime::currentDateTimeUtc(),
                    "ddd, dd MMM yyyy hh:mm:ss")
            .append(" GMT");
        addHeader("date", dateString.toLatin1());
    }
}

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
