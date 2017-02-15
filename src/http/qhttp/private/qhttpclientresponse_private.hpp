/** private imeplementation.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPCLIENT_RESPONSE_PRIVATE_HPP
#define QHTTPCLIENT_RESPONSE_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////

#include "httpreader.hxx"
#include "qhttpclient.hpp"
#include "qhttpclientresponse.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
class QHttpResponsePrivate :
    public details::HttpReader<details::HttpResponseBase>
{
    Q_DECLARE_PUBLIC(QHttpResponse)
    QHttpResponse* const    q_ptr;

public:
    explicit    QHttpResponsePrivate(QHttpClient* cli, QHttpResponse* q)
        : q_ptr(q), iclient(cli) {
        QHTTP_LINE_DEEPLOG
    }

    virtual    ~QHttpResponsePrivate() {
        QHTTP_LINE_DEEPLOG
    }

    void       initialize() {
    }

public:
    QString                 icustomStatusMessage;

protected:
    QHttpClient* const      iclient;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPCLIENT_RESPONSE_PRIVATE_HPP
