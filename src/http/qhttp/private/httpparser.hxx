/** @file httpparser.hxx
 *
 * @copyright (C) 2016
 * @date 2016.05.26
 * @version 1.0.0
 * @author amir zamani <azadkuh@live.com>
 *
 */

#ifndef __QHTTP_HTTPPARSER_HXX__
#define __QHTTP_HTTPPARSER_HXX__

#include "qhttpbase.hpp"
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace details {
///////////////////////////////////////////////////////////////////////////////


/// base HttpParser based on joyent http_parser
template<class TImpl>
class HttpParser
{
public:
    explicit HttpParser(http_parser_type type) {
        // create http_parser object
        iparser.data  = static_cast<TImpl*>(this);
        http_parser_init(&iparser, type);

        memset(&iparserSettings, 0, sizeof(http_parser_settings));
        iparserSettings.on_message_begin    = onMessageBegin;
        iparserSettings.on_url              = onUrl;
        iparserSettings.on_status           = onStatus;
        iparserSettings.on_header_field     = onHeaderField;
        iparserSettings.on_header_value     = onHeaderValue;
        iparserSettings.on_headers_complete = onHeadersComplete;
        iparserSettings.on_body             = onBody;
        iparserSettings.on_message_complete = onMessageComplete;
    }

    size_t parse(const char* data, size_t length) {
        return http_parser_execute(&iparser,
                &iparserSettings,
                data,
                length);
    }

public: // callback functions for http_parser_settings
    static int onMessageBegin(http_parser* p) {
        return me(p)->messageBegin(p);
    }

    static int onUrl(http_parser* p, const char* at, size_t length) {
        return me(p)->url(p, at, length);
    }

    static int onStatus(http_parser* p, const char* at, size_t length) {
        return me(p)->status(p, at, length);
    }

    static int onHeaderField(http_parser* p, const char* at, size_t length) {
        return me(p)->headerField(p, at, length);
    }

    static int onHeaderValue(http_parser* p, const char* at, size_t length) {
        return me(p)->headerValue(p, at, length);
    }

    static int onHeadersComplete(http_parser* p) {
        return me(p)->headersComplete(p);
    }

    static int onBody(http_parser* p, const char* at, size_t length) {
        return me(p)->body(p, at, length);
    }

    static int onMessageComplete(http_parser* p) {
        return me(p)->messageComplete(p);
    }


protected:
    // The ones we are reading in from the parser
    QByteArray           itempHeaderField;
    QByteArray           itempHeaderValue;
    // if connection has a timeout, these fields will be used
    quint32              itimeOut   = 0;
    QBasicTimer          itimer;
    // uniform socket object
    QSocket              isocket;
    // if connection should persist
    bool                 ikeepAlive = false;

    // joyent http_parser
    http_parser          iparser;
    http_parser_settings iparserSettings;

    static TImpl* me(http_parser* p) {
        return static_cast<TImpl*>(p->data);
    }
}; //

/// basic request parser (server)
template<class TImpl>
struct HttpRequestParser : public HttpParser<TImpl> {
    HttpRequestParser() : HttpParser<TImpl>(HTTP_REQUEST) {}
};

/// basic response parser (clinet)
template<class TImpl>
struct HttpResponseParser : public HttpParser<TImpl> {
    HttpResponseParser() : HttpParser<TImpl>(HTTP_RESPONSE) {}
};

///////////////////////////////////////////////////////////////////////////////
} // namespace details
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTP_HTTPPARSER_HXX__
