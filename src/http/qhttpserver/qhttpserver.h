/*
 * Copyright 2011 Nikhil Marathe <nsm.nikhil@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE. 
 */

#ifndef Q_HTTP_SERVER
#define Q_HTTP_SERVER

#define QHTTPSERVER_VERSION_MAJOR 0
#define QHTTPSERVER_VERSION_MINOR 1
#define QHTTPSERVER_VERSION_PATCH 0

#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

class QTcpServer;

class QHttpRequest;
class QHttpResponse;

/*!
 * A map of request or response headers
 */
typedef QHash<QString, QString> HeaderHash;

/*!
 * Maps status codes to string reason phrases
 */
extern QHash<int, QString> STATUS_CODES;

/*! \mainpage %QHttpServer Documentation
 *
 * \section introduction Introduction
 *
 * %QHttpServer is a easy to use, fast and light-weight
 * HTTP Server suitable for C++ web applications backed
 * by Qt. Since C++ web applications are pretty uncommon
 * the market for this project is pretty low.
 *
 * But integrating this with a module like QtScript
 * and using it to write JavaScript web applications is
 * a tempting possibility, and something that I want to
 * demonstrate at <a href="http://conf.kde.in">conf.kde.in 2011</a>.
 *
 * %QHttpServer uses a signal-slots based mechanism
 * for all communication, so no inheritance is required.
 * It tries to be as asynchronous as possible, to the
 * extent that request body data is also delivered as and
 * when it is received over the socket via signals. This
 * kind of programming may take some getting used to.
 *
 * %QHttpServer is backed by <a href="http://github.com/ry/http-parser">Ryan
 * Dahl's secure and fast http parser</a> which makes it streaming
 * till the lowest level.
 *
 * \section usage Usage
 *
 * Using %QHttpServer is very simple. Simply create a QHttpServer,
 * connect a slot to the newRequest() signal and use the request and
 * response objects.
 * See the QHttpServer class documentation for an example.
 *
 * \example helloworld/helloworld.cpp
 * \example helloworld/helloworld.h
 * \example greeting/greeting.cpp
 * \example greeting/greeting.h
 * \example bodydata/bodydata.cpp
 * \example bodydata/bodydata.h
 */

/*! \class QHttpServer
 * The QHttpServer class forms the basis of the %QHttpServer
 * project. It is a fast, non-blocking HTTP server.
 *
 * These are the steps to create a server and respond to requests.
 *
 * <ol>
 * <li>Create an instance of QHttpServer.</li>
 * <li>Connect a slot to the newRequest(QHttpRequest*, QHttpResponse*)
 * signal.</li>
 * <li>Create a QCoreApplication to drive the server event loop.</li>
 * <li>Respond to clients by writing out to the QHttpResponse object.</li>
 * </ol>
 *
 * helloworld.cpp
 * \include helloworld/helloworld.cpp
 * helloworld.h
 * \include helloworld/helloworld.h
 *
 */
class QHttpServer : public QObject
{
    Q_OBJECT

public:
    /*!
     * Create a new HTTP Server
     */
    QHttpServer(QObject *parent = 0);
    virtual ~QHttpServer();

    /*!
     * Start the server bound to the @c address and @c port.
     * This function returns immediately!
     *
     * \param address Address on which to listen to. Default is to listen on
     * all interfaces which means the server can be accessed from anywhere.
     * \param port Port number on which the server should run.
     * \return true if the server was started successfully, false otherwise.
     * \sa listen(quint16)
     */
    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port=0);

    /*!
     * Starts the server on @c port listening on all interfaces.
     *
     * \param port Port number on which the server should run.
     * \return true if the server was started successfully, false otherwise.
     * \sa listen(const QHostAddress&, quint16)
     */
    bool listen(quint16 port);

    /*!
     * Stop listening for connections
     */
    void close();

Q_SIGNALS:
    /*!
     * This signal is emitted whenever a client
     * makes a new request to the server.
     *
     * The slot should use the @c request and @c response
     * objects to communicate with the client.
     *
     * \section memorymanagement Memory Management
     *
     * The QHttpRequest and QHttpResponse deletion policies
     * are such.
     *
     * QHttpRequest is <strong>never</strong> deleted by %QHttpServer.
     * Since it is not possible to determine till what point the application
     * may want access to its data, it is up to the application to delete it.
     * A recommended way to handle this is to create a new responder object for
     * every request and to delete the request in that object's destructor. The
     * object itself can be deleted by connecting to QHttpResponse's done()
     * slot as explained below.
     *
     * You should <strong>NOT</strong> delete the QHttpRequest object until it
     * has emitted an QHttpRequest::end() signal.
     *
     * QHttpResponse queues itself up for auto-deletion once the application
     * calls its end() method. Once the data has been flushed to the underlying
     * socket, the object will emit a QHttpResponse::done() signal before queueing itself up
     * for deletion. You should <strong>NOT</strong> interact with the response
     * object once it has emitted QHttpResponse::done() although actual deletion does not
     * happen until QHttpResponse::destroyed() is emitted.
     * QHttpResponse::done() serves as a useful way to handle memory management of the
     * application itself. For example:
     *
     * \code
     * MyApp::MyApp()
     *     : QObject(0)
     * {
     *   QHttpServer *s = new QHttpServer;
     *   connect(s, SIGNAL(newRequest(...)), this, SLOT(handle(...)));
     *   s.listen(8000);
     * }
     *
     * void MyApp::handle(QHttpRequest *request, QHttpResponse *response)
     * {
     *   if( request->url() matches a route )
     *     new Responder(request, response);
     *   else
     *     new PageNotFound(request, response);
     * }
     *
     * ...
     *
     * Responder::Responder(QHttpRequest *request, QHttpResponse *response)
     * {
     *   m_request = request;
     *
     *   connect(request, SIGNAL(end()), response, SLOT(end()));
     *   // Once the request is complete, the response is ended.
     *   // when the response ends, it deletes itself
     *   // the Responder object connects to done()
     *   // which will lead to it being deleted
     *   // and this will delete the request.
     *   // So all 3 are properly deleted.
     *   connect(response, SIGNAL(done()), this, SLOT(deleteLater()));
     *   response->writeHead(200);
     *   response->write("Quitting soon");
     * }
     *
     * Responder::~Responder()
     * {
     *   delete m_request;
     *   m_request = 0;
     * }
     * \endcode
     *
     */
    void newRequest(QHttpRequest *request, QHttpResponse *response);

private Q_SLOTS:
    void newConnection();

private:
    QTcpServer *m_tcpServer;
};

#endif
