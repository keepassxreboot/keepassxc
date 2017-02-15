/** HTTP client class.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPCLIENT_HPP
#define QHTTPCLIENT_HPP

///////////////////////////////////////////////////////////////////////////////
#include "qhttpfwd.hpp"

#include <QTcpSocket>
#include <QUrl>

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
using TRequstHandler   = std::function<void (QHttpRequest*)>;
using TResponseHandler = std::function<void (QHttpResponse*)>;

/** a simple and async HTTP client class which sends a request to an HTTP server and parses the
 *  corresponding response.
 * This class internally handles the memory management and life cycle of QHttpRequest and
 *  QHttpResponse instances. you do not have to manually delete or keep their pointers.
 * in fact the QHttpRequest and QHttpResponse object will be deleted when the internal socket
 *  disconnects.
 */
class QHTTP_API QHttpClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint32 timeOut READ timeOut WRITE setTimeOut)

public:
    explicit    QHttpClient(QObject *parent = nullptr);

    virtual    ~QHttpClient();

    /** tries to connect to a HTTP server.
     *  when the connection is made, the reqHandler will be called
     *   and when the response is ready, resHandler will be called.
     * @note httpConnected() and newResponse() won't be emitted.
     *
     * @param method an HTTP method, ex: GET, POST, ...
     * @param url specifies server's address, port and optional path and query strings.
     *  if url starts with socket:// the request will be made on QLocalSocket, otherwise
     *  normal QTcpSocket will be used.
     * @param resHandler response handler (a lambda, std::function object, ...)
     * @return true if the url is valid or false (no connection will be made).
     */
    bool        request(THttpMethod method, QUrl url,
                        const TRequstHandler& reqHandler,
                        const TResponseHandler& resHandler);

    /** tries to connect to a HTTP server.
     *  when the connection is made, a default request handler is called automatically (
     *  simply calls req->end()) and when the response is ready, resHandler will be called.
     * @note httpConnected() and newResponse() won't be emitted.
     *
     * @param method an HTTP method, ex: GET, POST, ...
     * @param url specifies server's address, port and optional path and query strings.
     * @param resHandler response handler (a lambda, std::function object, ...)
     * @return true if the url is valid or false (no connection will be made).
     */
    inline bool request(THttpMethod method, QUrl url, const TResponseHandler& resHandler) {
        return request(method, url, nullptr, resHandler);
    }

    /** tries to connect to a HTTP server.
     *  when the connection is made, creates and emits a QHttpRequest instance
     *   by @sa httpConnected(QHttpRequest*).
     * @note both httpConnected() and newResponse() may be emitted.
     *
     * @param method an HTTP method, ex: GET, POST, ...
     * @param url specifies server's address, port and optional path and query strings.
     * @return true if the url is valid or false (no connection will be made).
     */
    inline bool request(THttpMethod method, QUrl url) {
        return request(method, url, nullptr, nullptr);
    }

    /** checks if the connetion to the server is open. */
    bool        isOpen() const;

    /** forcefully close the connection. */
    void        killConnection();


    /** returns time-out value [mSec] for ESTABLISHED connections (sockets).
     *  @sa setTimeOut(). */
    quint32     timeOut()const;

    /** set time-out for ESTABLISHED connections in miliseconds [mSec].
     * each (already opened) connection will be forcefully closed after this timeout.
     *  a zero (0) value disables timer for new connections. */
    void        setTimeOut(quint32);

    /** set a time-out [mSec] for making a new connection (make a request).
     * if connecting to server takes more than this time-out value,
     *  the @sa timedOut(quint32) signal will be emitted and connection will be killed.
     * 0 (default) timeout value means to disable this timer.
     */
    void        setConnectingTimeOut(quint32);

    template<class Handler>
    void        setConnectingTimeOut(quint32 timeout, Handler&& func) {
        setConnectingTimeOut(timeout);
        QObject::connect(this, &QHttpClient::connectingTimeOut,
                std::forward<Handler&&>(func)
                );
    }

    /** returns the backend type of this client. */
    TBackend    backendType() const;

    /** returns tcp socket of the connection if backend() == ETcpSocket. */
    QTcpSocket* tcpSocket() const;

    /** returns local socket of the connection if backend() == ELocalSocket. */
    QLocalSocket* localSocket() const;

signals:
    /** emitted when a new HTTP connection to the server is established.
     * if you overload onRequestReady this signal won't be emitted.
     * @sa onRequestReady
     * @sa QHttpRequest
     */
    void        httpConnected(QHttpRequest* req);

    /** emitted when a new response is received from the server.
     * if you overload onResponseReady this signal won't be emitted.
     * @sa onResponseReady
     * @sa QHttpResponse
     */
    void        newResponse(QHttpResponse* res);

    /** emitted when the HTTP connection drops or being disconnected. */
    void        disconnected();

    /// emitted when fails to connect to a HTTP server. @sa setConnectingTimeOut()
    void        connectingTimeOut();


protected:
    /** called when a new HTTP connection is established.
     * you can overload this method, the default implementaion only emits connected().
     * @param req use this request instance for assinging the
     *   request headers and sending optional body.
     * @see httpConnected(QHttpRequest*)
     */
    virtual void onRequestReady(QHttpRequest* req);

    /** called when a new response is received from the server.
     *  you can overload this method, the default implementaion only emits newResponse().
     * @param res use this instance for reading incoming response.
     * @see newResponse(QHttpResponse*)
     */
    virtual void onResponseReady(QHttpResponse* res);

protected:
    explicit    QHttpClient(QHttpClientPrivate&, QObject*);

    void        timerEvent(QTimerEvent*) override;

    Q_DECLARE_PRIVATE(QHttpClient)
    Q_DISABLE_COPY(QHttpClient)
    QScopedPointer<QHttpClientPrivate>  d_ptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // define QHTTPCLIENT_HPP
