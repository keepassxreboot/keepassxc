/** HTTP request from a client.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPCLIENT_REQUEST_HPP
#define QHTTPCLIENT_REQUEST_HPP

///////////////////////////////////////////////////////////////////////////////
#include "qhttpabstracts.hpp"
#include <QUrl>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
/** a class for building a new HTTP request.
 * the life cycle of this class and the memory management is handled by QHttpClient.
 * @sa QHttpClient
 */
class QHTTP_API QHttpRequest : public QHttpAbstractOutput
{
    Q_OBJECT

public:
    virtual        ~QHttpRequest();

public: // QHttpAbstractOutput methods:
    /** @see QHttpAbstractOutput::setVersion(). */
    void            setVersion(const QString& versionString) override;

    /** @see QHttpAbstractOutput::addHeader(). */
    void            addHeader(const QByteArray& field, const QByteArray& value) override;

    /** @see QHttpAbstractOutput::headers(). */
    THeaderHash&    headers() override;

    /** @see QHttpAbstractOutput::write(). */
    void            write(const QByteArray &data) override;

    /** @see QHttpAbstractOutput::end(). */
    void            end(const QByteArray &data = QByteArray()) override;

public:
    /** returns parent QHttpClient object. */
    QHttpClient*    connection() const;

protected:
    explicit        QHttpRequest(QHttpClient*);
    explicit        QHttpRequest(QHttpRequestPrivate&, QHttpClient*);
    friend class    QHttpClient;

    Q_DECLARE_PRIVATE(QHttpRequest)
    QScopedPointer<QHttpRequestPrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // define QHTTPCLIENT_REQUEST_HPP
