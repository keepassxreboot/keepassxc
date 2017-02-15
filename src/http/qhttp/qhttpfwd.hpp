/** forward declarations and general definitions of the QHttp.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPFWD_HPP
#define QHTTPFWD_HPP
///////////////////////////////////////////////////////////////////////////////
#include <QHash>
#include <QString>
#include <QtGlobal>

#include <functional>
///////////////////////////////////////////////////////////////////////////////
// Qt
class QTcpServer;
class QTcpSocket;
class QLocalServer;
class QLocalSocket;

// http_parser
struct http_parser_settings;
struct http_parser;

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
///////////////////////////////////////////////////////////////////////////////

/** A map of request or response headers. */
class THeaderHash : public QHash<QByteArray, QByteArray>
{
public:
    /** checks for a header item, regardless of the case of the characters. */
    bool    has(const QByteArray& key) const {
        return contains(key.toLower());
    }

    /** checks if a header has the specified value ignoring the case of the characters. */
    bool    keyHasValue(const QByteArray& key, const QByteArray& value) const {
        if ( !contains(key) )
            return false;

        const QByteArray& v = QHash<QByteArray, QByteArray>::value(key);
        return qstrnicmp(value.constData(), v.constData(), v.size()) == 0;
    }
};

/// QHash/QMap iterators are incompatibility with range for
template<class Iterator, class Func>
void for_each(Iterator first, Iterator last, Func&& f) {
    while ( first != last ) {
        f( first );
        ++first;
    }
}


/** Request method enumeration.
 * @note Taken from http_parser.h */
enum THttpMethod {
    EHTTP_DELETE         =  0,        ///< DELETE
    EHTTP_GET            =  1,        ///< GET
    EHTTP_HEAD           =  2,        ///< HEAD
    EHTTP_POST           =  3,        ///< POST
    EHTTP_PUT            =  4,        ///< PUT
    /* pathological */
    EHTTP_CONNECT        =  5,        ///< CONNECT
    EHTTP_OPTIONS        =  6,        ///< OPTIONS
    EHTTP_TRACE          =  7,        ///< TRACE
    /* webdav */
    EHTTP_COPY           =  8,        ///< COPY
    EHTTP_LOCK           =  9,        ///< LOCK
    EHTTP_MKCOL          = 10,        ///< MKCOL
    EHTTP_MOVE           = 11,        ///< MOVE
    EHTTP_PROPFIND       = 12,        ///< PROPFIND
    EHTTP_PROPPATCH      = 13,        ///< PROPPATCH
    EHTTP_SEARCH         = 14,        ///< SEARCH
    EHTTP_UNLOCK         = 15,        ///< UNLOCK
    EHTTP_BIND           = 16,        ///< BIND
    EHTTP_REBIND         = 17,        ///< REBIND
    EHTTP_UNBIND         = 18,        ///< UNBIND
    EHTTP_ACL            = 19,        ///< ACL
    /* subversion */
    EHTTP_REPORT         = 20,        ///< REPORT
    EHTTP_MKACTIVITY     = 21,        ///< MKACTIVITY
    EHTTP_CHECKOUT       = 22,        ///< CHECKOUT
    EHTTP_MERGE          = 23,        ///< MERGE
    /* upnp */
    EHTTP_MSEARCH        = 24,        ///< M-SEARCH
    EHTTP_NOTIFY         = 25,        ///< NOTIFY
    EHTTP_SUBSCRIBE      = 26,        ///< SUBSCRIBE
    EHTTP_UNSUBSCRIBE    = 27,        ///< UNSUBSCRIBE
    /* RFC-5789 */
    EHTTP_PATCH          = 28,        ///< PATCH
    EHTTP_PURGE          = 29,        ///< PURGE
    /* CalDAV */
    EHTTP_MKCALENDAR     = 30,        ///< MKCALENDAR
    /* RFC-2068, section 19.6.1.2 */
    EHTTP_LINK           = 31,        ///< LINK
    EHTTP_UNLINK         = 32,        ///< UNLINK
};

/** HTTP status codes. */
enum TStatusCode {
     ESTATUS_CONTINUE                           = 100,
     ESTATUS_SWITCH_PROTOCOLS                   = 101,
     ESTATUS_OK                                 = 200,
     ESTATUS_CREATED                            = 201,
     ESTATUS_ACCEPTED                           = 202,
     ESTATUS_NON_AUTHORITATIVE_INFORMATION      = 203,
     ESTATUS_NO_CONTENT                         = 204,
     ESTATUS_RESET_CONTENT                      = 205,
     ESTATUS_PARTIAL_CONTENT                    = 206,
     ESTATUS_MULTI_STATUS                       = 207,
     ESTATUS_MULTIPLE_CHOICES                   = 300,
     ESTATUS_MOVED_PERMANENTLY                  = 301,
     ESTATUS_FOUND                              = 302,
     ESTATUS_SEE_OTHER                          = 303,
     ESTATUS_NOT_MODIFIED                       = 304,
     ESTATUS_USE_PROXY                          = 305,
     ESTATUS_TEMPORARY_REDIRECT                 = 307,
     ESTATUS_BAD_REQUEST                        = 400,
     ESTATUS_UNAUTHORIZED                       = 401,
     ESTATUS_PAYMENT_REQUIRED                   = 402,
     ESTATUS_FORBIDDEN                          = 403,
     ESTATUS_NOT_FOUND                          = 404,
     ESTATUS_METHOD_NOT_ALLOWED                 = 405,
     ESTATUS_NOT_ACCEPTABLE                     = 406,
     ESTATUS_PROXY_AUTHENTICATION_REQUIRED      = 407,
     ESTATUS_REQUEST_TIMEOUT                    = 408,
     ESTATUS_CONFLICT                           = 409,
     ESTATUS_GONE                               = 410,
     ESTATUS_LENGTH_REQUIRED                    = 411,
     ESTATUS_PRECONDITION_FAILED                = 412,
     ESTATUS_REQUEST_ENTITY_TOO_LARGE           = 413,
     ESTATUS_REQUEST_URI_TOO_LONG               = 414,
     ESTATUS_REQUEST_UNSUPPORTED_MEDIA_TYPE     = 415,
     ESTATUS_REQUESTED_RANGE_NOT_SATISFIABLE    = 416,
     ESTATUS_EXPECTATION_FAILED                 = 417,
     ESTATUS_INTERNAL_SERVER_ERROR              = 500,
     ESTATUS_NOT_IMPLEMENTED                    = 501,
     ESTATUS_BAD_GATEWAY                        = 502,
     ESTATUS_SERVICE_UNAVAILABLE                = 503,
     ESTATUS_GATEWAY_TIMEOUT                    = 504,
     ESTATUS_HTTP_VERSION_NOT_SUPPORTED         = 505
};

/** The backend of QHttp library. */
enum TBackend {
    ETcpSocket     = 0, ///< client / server work on top of TCP/IP stack. (default)
    ESslSocket     = 1, ///< client / server work on SSL/TLS tcp stack. (not implemented yet)
    ELocalSocket   = 2  ///< client / server work on local socket (unix socket).
};

///////////////////////////////////////////////////////////////////////////////
namespace server {
///////////////////////////////////////////////////////////////////////////////
class QHttpServer;
class QHttpConnection;
class QHttpRequest;
class QHttpResponse;

// Privte classes
class QHttpServerPrivate;
class QHttpConnectionPrivate;
class QHttpRequestPrivate;
class QHttpResponsePrivate;

using TServerHandler = std::function<void (QHttpRequest*, QHttpResponse*)>;

///////////////////////////////////////////////////////////////////////////////
} // namespace server
///////////////////////////////////////////////////////////////////////////////
namespace client {
///////////////////////////////////////////////////////////////////////////////
class QHttpClient;
class QHttpRequest;
class QHttpResponse;

// Private classes
class QHttpClientPrivate;
class QHttpRequestPrivate;
class QHttpResponsePrivate;
///////////////////////////////////////////////////////////////////////////////
} // namespace client
///////////////////////////////////////////////////////////////////////////////
#ifdef Q_OS_WIN
#   if defined(QHTTP_EXPORT)
#       define QHTTP_API __declspec(dllexport)
#   else
#       define QHTTP_API __declspec(dllimport)
#   endif
#else
#   define QHTTP_API
#endif


#if QHTTP_MEMORY_LOG > 0
#   define QHTTP_LINE_LOG fprintf(stderr, "%s(): obj = %p    @ %s[%d]\n",\
    __FUNCTION__, this, __FILE__, __LINE__);
#else
#   define QHTTP_LINE_LOG
#endif

#if QHTTP_MEMORY_LOG > 1
#   define QHTTP_LINE_DEEPLOG QHTTP_LINE_LOG
#else
#   define QHTTP_LINE_DEEPLOG
#endif
///////////////////////////////////////////////////////////////////////////////
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // define QHTTPFWD_HPP
