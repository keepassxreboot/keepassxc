/** HTTP connection class.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_CONNECTION_HPP
#define QHTTPSERVER_CONNECTION_HPP
///////////////////////////////////////////////////////////////////////////////
#include "qhttpfwd.hpp"

#include <QObject>

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
/** a HTTP connection in the server.
 * this class controls the HTTP connetion and handles life cycle and the memory management
 *  of QHttpRequest and QHttpResponse instances autoamtically.
 */
class QHTTP_API QHttpConnection : public QObject
{
    Q_OBJECT

public:
    virtual        ~QHttpConnection();

    /** set an optional timer event to close the connection. */
    void            setTimeOut(quint32 miliSeconds);

    /** forcefully kills (closes) a connection. */
    void            killConnection();

    /** optionally set a handler for connection class.
     * @note if you set this handler, the newRequest() signal won't be emitted.
     */
    void            onHandler(const TServerHandler& handler);

    /** returns the backend type of the connection. */
    TBackend        backendType() const;

    /** returns connected socket if the backend() == ETcpSocket. */
    QTcpSocket*     tcpSocket() const;

    /** returns connected socket if the backend() == ELocalSocket. */
    QLocalSocket*   localSocket() const;

    /** creates a new QHttpConnection based on arguments. */
    static
    QHttpConnection* create(qintptr sokDescriptor, TBackend backendType, QObject* parent) {
        QHttpConnection* conn = new QHttpConnection(parent);
        conn->setSocketDescriptor(sokDescriptor, backendType);
        return conn;
    }

signals:
    /** emitted when a pair of HTTP request and response are ready to interact.
     * @param req incoming request by the client.
     * @param res outgoing response to the client.
     */
    void            newRequest(QHttpRequest* req, QHttpResponse* res);

    /** emitted when the tcp/local socket, disconnects. */
    void            disconnected();

protected:
    explicit        QHttpConnection(QObject *parent);
    explicit        QHttpConnection(QHttpConnectionPrivate&, QObject *);

    void            setSocketDescriptor(qintptr sokDescriptor, TBackend backendType);
    void            timerEvent(QTimerEvent*) override;

    Q_DISABLE_COPY(QHttpConnection)
    Q_DECLARE_PRIVATE(QHttpConnection)
    QScopedPointer<QHttpConnectionPrivate>    d_ptr;

    friend class    QHttpServer;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // #define QHTTPSERVER_CONNECTION_HPP
