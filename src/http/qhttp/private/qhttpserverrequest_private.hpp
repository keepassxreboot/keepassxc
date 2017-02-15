/** private imeplementation.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_REQUEST_PRIVATE_HPP
#define QHTTPSERVER_REQUEST_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////

#include "httpreader.hxx"
#include "qhttpserverrequest.hpp"
#include "qhttpserverconnection.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
class QHttpRequestPrivate :
    public details::HttpReader<details::HttpRequestBase>
{
protected:
    Q_DECLARE_PUBLIC(QHttpRequest)
    QHttpRequest* const     q_ptr;

public:
    explicit    QHttpRequestPrivate(QHttpConnection* conn, QHttpRequest* q) : q_ptr(q), iconnection(conn) {
        QHTTP_LINE_DEEPLOG
    }

    virtual    ~QHttpRequestPrivate() {
        QHTTP_LINE_DEEPLOG
    }

    void        initialize() {
    }

public:
    QString     iremoteAddress;
    quint16     iremotePort = 0;

    QHttpConnection* const  iconnection = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPSERVER_REQUEST_PRIVATE_HPP
