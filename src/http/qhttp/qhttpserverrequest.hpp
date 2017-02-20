/** HTTP request which is received by the server.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_REQUEST_HPP
#define QHTTPSERVER_REQUEST_HPP
///////////////////////////////////////////////////////////////////////////////

#include "qhttpabstracts.hpp"

#include <QUrl>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
/** The QHttpRequest class represents the header and body data sent by the client.
 * The class is <b>read-only</b>.
 * @sa QHttpConnection
 */
class QHTTP_API QHttpRequest : public QHttpAbstractInput
{
    Q_OBJECT

public:
    virtual            ~QHttpRequest();

public: // QHttpAbstractInput methods:
    /** @see QHttpAbstractInput::headers(). */
    const THeaderHash&  headers() const override;

    /** @see QHttpAbstractInput::httpVersion(). */
    const QString&      httpVersion() const override;

    /** @see QHttpAbstractInput::isSuccessful(). */
    bool                isSuccessful() const override;

    /** @see QHttpAbstractInput::collectData(). */
    void                collectData(int atMost = -1) override;

    /** @see QHttpAbstractInput::collectedData(). */
    const QByteArray&   collectedData()const override;


public:
    /** The method used for the request. */
    THttpMethod         method() const ;

    /** Returns the method string for the request.
     * @note This will plainly transform the enum into a string HTTP_GET -> "HTTP_GET". */
    const QString       methodString() const;

    /** The complete URL for the request.
     * This includes the path and query string. @sa path(). */
    const QUrl&         url() const;

    /** IP Address of the client in dotted decimal format. */
    const QString&      remoteAddress() const;

    /** Outbound connection port for the client. */
    quint16             remotePort() const;

    /** returns the parent QHttpConnection object. */
    QHttpConnection*    connection() const;

protected:
    explicit            QHttpRequest(QHttpConnection*);
    explicit            QHttpRequest(QHttpRequestPrivate&, QHttpConnection*);
    friend class        QHttpConnectionPrivate;

    Q_DECLARE_PRIVATE(QHttpRequest)
    QScopedPointer<QHttpRequestPrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // define QHTTPSERVER_REQUEST_HPP
