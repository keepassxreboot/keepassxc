/** interfaces of QHttp' incomming and outgoing classes.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPABSTRACTS_HPP
#define QHTTPABSTRACTS_HPP

///////////////////////////////////////////////////////////////////////////////
#include "qhttpfwd.hpp"

#include <QObject>
#include <functional>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
///////////////////////////////////////////////////////////////////////////////

/** a utility class to give the string representation of qhttp types. */
class QHTTP_API Stringify
{
public:
    /** returns the standard message for an HTTP status code. */
    static const char* toString(TStatusCode);

    /** returns the standars name of an HTTP method. */
    static const char* toString(THttpMethod);
};

///////////////////////////////////////////////////////////////////////////////

/** an interface for input (incoming) HTTP packets.
 * server::QHttpRequest or client::QHttpResponse inherit from this class. */
class QHTTP_API QHttpAbstractInput : public QObject
{
    Q_OBJECT

public:
    /** Return all the headers in the incomming packet.
     * This returns a reference. If you want to store headers
     *  somewhere else, where the request may be deleted,
     *  make sure you store them as a copy.
     * @note All header names are <b>lowercase</b> . */
    virtual const THeaderHash&  headers() const=0;

    /** The HTTP version of the packet.
     * @return A string in the form of "x.x" */
    virtual const QString&      httpVersion() const=0;

    /** If this packet was successfully received.
     * Set before end() has been emitted, stating whether
     *  the message was properly received. This is false
     *  until the receiving the full request has completed. */
    virtual bool                isSuccessful() const=0;

signals:
    /** Emitted when new body data has been received.
     * @param data Received data.
     * @note This may be emitted zero or more times depending on the transfer type.
     * @see onData();
     */
    void                        data(QByteArray data);

    /** Emitted when the incomming packet has been fully received.
     * @note The no more data() signals will be emitted after this.
     * @see onEnd();
     */
    void                        end();

public:
    /** optionally set a handler for data() signal.
     * @param dataHandler a std::function or lambda handler to receive incoming data.
     * @note if you set this handler, the data() signal won't be emitted anymore.
     */
    template<class Func>
    void                        onData(Func f) {
        QObject::connect(this, &QHttpAbstractInput::data, f);
    }


    /** optionally set a handler for end() signal.
     * @param endHandler a std::function or lambda handler to receive end notification.
     * @note if you set this handler, the end() signal won't be emitted anymore.
     */
    template<class Func>
    void                        onEnd(Func f) {
        QObject::connect(this, &QHttpAbstractInput::end, f);
    }

public:
    /** tries to collect all the incoming data internally.
     * @note if you call this method, data() signal won't be emitted and
     *  onData() will have no effect.
     *
     * @param atMost maximum acceptable incoming data. if the incoming data
     *  exceeds this value, the connection won't read any more data and
     *  end() signal will be emitted.
     *  default value (-1) means read data as "content-length" or unlimited if
     *  the body size is unknown.
     */
    virtual void                collectData(int atMost = -1) =0;

    /** returns the collected data requested by collectData(). */
    virtual const QByteArray&   collectedData()const =0;


public:
    virtual                    ~QHttpAbstractInput() = default;

    explicit                    QHttpAbstractInput(QObject* parent);

    Q_DISABLE_COPY(QHttpAbstractInput)
};

///////////////////////////////////////////////////////////////////////////////

/** an interface for output (outgoing) HTTP packets.
 * server::QHttpResponse or client::QHttpRequest inherit from this class. */
class QHTTP_API QHttpAbstractOutput : public QObject
{
    Q_OBJECT

public:
    /** changes the HTTP version string ex: "1.1" or "1.0".
     * version is "1.1" set by default. */
    virtual void            setVersion(const QString& versionString)=0;

    /** helper function. @sa addHeader */
    template<typename T>
    void                    addHeaderValue(const QByteArray &field, T value);

    /** adds an HTTP header to the packet.
     * @note this method does not actually write anything to socket, just prepares the headers(). */
    virtual void            addHeader(const QByteArray& field, const QByteArray& value)=0;

    /** returns all the headers that already been set. */
    virtual THeaderHash&    headers()=0;

    /** writes a block of data into the HTTP packet.
     * @note headers are written (flushed) before any data.
     * @warning after calling this method add a new header, set staus code, set Url have no effect! */
    virtual void            write(const QByteArray &data)=0;

    /** ends (finishes) the outgoing packet by calling write().
     * headers and data will be flushed to the underlying socket.
     *
     * @sa write() */
    virtual void            end(const QByteArray &data = QByteArray())=0;

signals:
    /** Emitted when all the data has been sent.
     * this signal indicates that the underlaying socket has transmitted all
     *  of it's buffered data. */
    void                    allBytesWritten();

    /** Emitted when the packet is finished and reports if it was the last packet.
     * if it was the last packet (google for "Connection: keep-alive / close")
     *  the http connection (socket) will be closed automatically. */
    void                    done(bool wasTheLastPacket);

public:
    virtual                ~QHttpAbstractOutput() = default;

protected:
    explicit                QHttpAbstractOutput(QObject* parent);

    Q_DISABLE_COPY(QHttpAbstractOutput)
};

template<> inline void
QHttpAbstractOutput::addHeaderValue<int>(const QByteArray &field, int value) {
    addHeader(field, QString::number(value).toLatin1());
}

template<> inline void
QHttpAbstractOutput::addHeaderValue<size_t>(const QByteArray &field, size_t value) {
    addHeader(field, QString::number(value).toLatin1());
}

template<> inline void
QHttpAbstractOutput::addHeaderValue<QString>(const QByteArray &field, QString value) {
    addHeader(field, value.toUtf8());
}

///////////////////////////////////////////////////////////////////////////////
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPABSTRACTS_HPP
