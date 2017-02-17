/** @file httpwriter.hxx
 *
 * @copyright (C) 2016
 * @date 2016.05.26
 * @version 1.0.0
 * @author amir zamani <azadkuh@live.com>
 *
 */

#ifndef __QHTTP_HTTPWRITER_HXX__
#define __QHTTP_HTTPWRITER_HXX__

#include "qhttpbase.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace details {
///////////////////////////////////////////////////////////////////////////////

// usage in client::QHttpRequest, server::QHttpResponse
template<class TBase, class TImpl>
class HttpWriter : public TBase
{
public:
    bool addHeader(const QByteArray &field, const QByteArray &value) {
        if ( ifinished )
            return false;

        TBase::iheaders.insert(field.toLower(), value);
        return true;
    }

    bool writeHeader(const QByteArray& field, const QByteArray& value) {
        if ( ifinished )
            return false;

        QByteArray buffer = QByteArray(field)
                            .append(": ")
                            .append(value)
                            .append("\r\n");

        isocket.writeRaw(buffer);
        return true;
    }

    bool writeData(const QByteArray& data) {
        if ( ifinished )
            return false;

        ensureWritingHeaders();
        isocket.writeRaw(data);
        return true;
    }

    bool endPacket(const QByteArray& data) {
        if ( !writeData(data) )
            return false;

        isocket.flush();
        ifinished = true;
        return true;
    }

    void ensureWritingHeaders() {
        if ( ifinished    ||    iheaderWritten )
            return;

        TImpl* me = static_cast<TImpl*>(this);
        isocket.writeRaw(me->makeTitle());
        writeHeaders();

        iheaderWritten = true;
    }

    void writeHeaders(bool doFlush = false) {
        if ( ifinished    ||    iheaderWritten )
            return;

        if ( TBase::iheaders.keyHasValue("connection", "keep-alive") )
            ikeepAlive = true;
        else
            TBase::iheaders.insert("connection", "close");

        TImpl* me = static_cast<TImpl*>(this);
        me->prepareHeadersToWrite();

        for ( auto cit = TBase::iheaders.constBegin(); cit != TBase::iheaders.constEnd(); cit++ ) {
            const QByteArray& field = cit.key();
            const QByteArray& value = cit.value();

            writeHeader(field, value);
        }

        isocket.writeRaw("\r\n");
        if ( doFlush )
            isocket.flush();
    }

public:
    QSocket isocket;

    bool    ifinished      = false;
    bool    iheaderWritten = false;
    bool    ikeepAlive     = false;
};


///////////////////////////////////////////////////////////////////////////////
} // namespace details
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTP_HTTPWRITER_HXX__
