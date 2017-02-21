# QHttp


### Table of contents
- [About](#about)
- [Sample codes](#sample-codes)
- [Features](#features)
- [Setup](#setup)
- [Multi-threading](#multi-threading)
- [Source tree](#source-tree)
- [Disclaimer](#disclaimer)
- [License](#license)

## About
[TOC](#table-of-contents)

`QHttp` is a lightweight, asynchronous and fast HTTP library, containing both server and client side classes for managing connections, parsing and building HTTP requests and responses. this project is inspired by [nikhilm/qhttpserver](https://github.com/nikhilm/qhttpserver) effort to implement a Qt HTTP server. `QHttp` pushes the idea further by implementing client classes and better memory management, a lot more Node.js-like API, ...

* the fantastic [nodejs/http-parser](https://github.com/nodejs/http-parser) is the core parser of HTTP requests (server mode) and responses (client mode).

* By using `std::function` and `c++11 lambda`, the API is intentionally similar to the [Node.js' http module](http://nodejs.org/api/http.html). Asynchronous and non-blocking HTTP programming is quite easy with `QHttp`. have a look at [sample codes](#sample-codes).

* the objective of `QHttp` is being light weight with a simple API for Qt developers to implement RESTful web services in private (internal) zones. [more](#disclaimer)



## Sample codes
[TOC](#table-of-contents)

a HelloWorld **HTTP server** by `QHttp` looks like:
``` cpp
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    using namespace qhttp::server;
    QHttpServer server(&app);
    // listening on 0.0.0.0:8080
    server.listen(QHostAddress::Any, 8080, [](QHttpRequest* req, QHttpResponse* res) {

        res->setStatusCode(qhttp::ESTATUS_OK);      // http status 200
        //res->addHeader("connection", "close");    // optional, it's the default header
        res->end("Hello World!\n");                 // the response body data
        // by "connection: close", the req and res objects will be deleted automatically.
    });

    if ( !server.isListening() ) {
        fprintf(stderr, "failed. can not listen at port 8080!\n");
        return -1;
    }

    return app.exec();
}
```

to request weather information by **HTTP client**:
```cpp
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    using namespace qhttp::client;
    QHttpClient  client(&app);
    QByteArray   httpBody;

    QUrl weatherUrl("http://api.openweathermap.org/data/2.5/weather?q=tehran,ir&units=metric&mode=xml");

    client.request(qhttp::EHTTP_GET, weatherUrl, [&httpBody](QHttpResponse* res) {
        // response handler, called when the HTTP headers of the response are ready

        // gather HTTP response data
        res->onData([&httpBody](const QByteArray& chunk) {
            httpBody.append(chunk);
        });

        // called when all data in HTTP response have been read.
        res->onEnd([&httpBody]() {
            // print the XML body of the response
            puts("\n[incoming response:]");
            puts(httpBody.constData());
            puts("\n\n");

            QCoreApplication::instance()->quit();
        });

        // just for fun! print incoming headers:
        puts("\n[Headers:]");
        const qhttp::THeaderHash& hs = res->headers();
        for ( auto cit = hs.constBegin(); cit != hs.constEnd(); cit++) {
            printf("%s : %s\n", cit.key().constData(), cit.value().constData());
        }
    });

    // set a timeout for making the request
    client.setConnectingTimeOut(10000, []{
        qDebug("connecting to HTTP server timed out!");
        QCoreApplication::quit();
    });


    return app.exec();
}
```


## Features
[TOC](#table-of-contents)

* the only dependencies are: [Qt 5](http://qt-project.org/downloads), [c++11](http://en.wikipedia.org/wiki/C%2B%2B11) and [joyent/http-parser](https://github.com/joyent/http-parser)

* both TCP and UNIX (local) sockets are supported as backend.

* separate `namespace`s for server and client classes.

* HTTP server classes: [QHttpServer](./src/qhttpserver.hpp), [QHttpConnection](./src/qhttpserverconnection.hpp), [QHttpRequest](./src/qhttpserverrequest.hpp) and [QHttpResponse](./src/qhttpserverresponse.hpp).

* HTTP client classes: [QHttpClient](./src/qhttpclient.hpp), [QHttpRequest](./src/qhttpclientrequest.hpp) and [QHttpResponse](./src/qhttpclientresponse.hpp).

* **automatic memory management** of objects. Instances of connections, requests and replies will be deleted automatically when socket drops or disconnected.

* **PIMPL** (Private classes) to achieve better ABI compatibility and cleaner API.

* **Asynchronous** and **non-blocking**. You can handle thousands of concurrent HTTP connections efficiently by a single thread, although a multi-threaded HTTP server is easy to implement.

* **high throughput**, I have tried the `QHttp` and [gason++](https://github.com/azadkuh/gason--) to implement a REST/Json web service on an Ubuntu VPS (dual core + 512MB ram) with more than **5800** connections per second (stress test). On a MacBook Pro (i5 4258U 4cores with HT + 8096MB ram), `QHttp` easily reaches to more than **11700** connections / second. Generally `QHttp` is **1.5x ~ 3x** faster than `Node.js` depending on your machine / OS. check [benchmark app](./example/benchmard/README.md) to measure your system.

* Tested under **Linux** (Ubuntu 12.04 LTS, 14.04 LTS, g++) and **OS X** (10.9/10.10/10.11, clang). Easily portable where ever Qt 5 works. (tested by some users on Windows7/msvc2013 and Windows8.1/msvc2015)


## Setup
[TOC](#table-of-contents)

instructions:
```bash
# first clone this repository:
$> git clone --depth=1 https://github.com/azadkuh/qhttp.git -b master
$> cd qhttp

# prepare dependencies:
$> ./update-dependencies.sh

# now build the library and the examples
$> qmake qhttp.pro
$> make -j 8
```

## Multi-threading
[TOC](#table-of-contents)

As `QHttp` is **asynchronous** and **non-blocking**, your app can handle thousands of concurrent HTTP connections by a single thread.

in some rare scenarios you may want to use multiple handler threads (although it's not the best solution):

* there are some blocking APIs (QSql, system calls, ...) in your connection handler (adopting asynchronous layer over the blocking API is a better approach).

* the hardware has lots of free cores and the measurement shows that the load on the main `QHttp` thread is close to highest limit. There you can spawn some other handler threads.


[benchmark example](./example/benchmark/README.md) shows how to implement a single or multi threaded HTTP app (both server and client). This example uses worker `QThread` and `QObject::moveToThread()` for worker objects. see also: [Subclassing no longer recommended way of using QThread](http://qt-project.org/doc/note_revisions/5/8/view).

**Note**:
> moving objects between threads is an expensive job, more ever the locking/unlocking mechanism, creating or stopping threads, ... cost even more! so using multiple threads in an application is not guaranteed to get better performance, but it's guaranteed to add more complexity, nasty bugs and headache!

see why other top performer networking libraries as ZeroMQ are concurrent but not multi-threaded by default:

* [ZeroMQ : Multithreading Magic](http://zeromq.org/blog:multithreading-magic)
* [Node.js : about](http://nodejs.org/about/)


## Source tree
[TOC](#table-of-contents)


* **`3rdparty/`**:
will contain `http-parser` source tree as the only dependency.
this directory is created by setup. see also: [setup](#setup).

* **`example/`**:
contains some sample applications representing the `QHttp` usage:
    * **`helloworld/`**:
    the HelloWorld example of `QHttp`, both server + client are represented.
    see: [README@helloworld](./example/helloworld/README.md)

    * **`basic-server/`**:
    a basic HTTP server shows how to collect the request body, and respond to the clients.
    see: [README@basic-server](./example/basic-server/README.md)


    * **`benchmark/`**:
    a simple utility to measure the throughput (requests per second) of `QHttp` as a REST/Json server. this app provides both the server and attacking clients.
    see: [README@benchmark](./example/benchmark/README.md)

    * **`nodejs/`**:
    Node.js implementation of `benchmark/` in server mode. Provided for benchmarking `QHttp` with `Node.js` as a RESTFul service app.
    see: [README@nodejs](./example/nodejs/README.md)


* **`src/`**:
holds the source code of `QHttp`. server classes are prefixed by `qhttpserver*` and client classes by `qhttpclient*`.
    * **`private/`**:
    Private classes of the library. see: [d-pointers](https://qt-project.org/wiki/Dpointer).

* **`tmp/`**:
a temporary directory which is created while `make`ing the library and holds all the `.o`, `moc files`, etc.

* **`xbin/`**:
all the executable and binaries will be placed on this folder by `make`.




## Disclaimer
[TOC](#table-of-contents)

* Implementing a lightweight and simple HTTP server/client in Qt with Node.js like API, is the main purpose of `QHttp`.

* There are lots of features in a full blown HTTP server which are out of scope of this small library, although those can be added on top of `QHttp`.

* The client classes are by no mean designed as a `QNetworkAccessManager` replacement. `QHttpClient` is simpler and lighter, for serious scenarios just use `QNetworkAccessManager`.

* I'm a busy person.


> If you have any ideas, critiques, suggestions or whatever you want to call it, please open an issue. I'll be happy to hear from you what you'd see in this lib. I think about all suggestions, and I try to add those that make sense.


## License
[TOC](#table-of-contents)

Distributed under the MIT license. Copyright (c) 2014, Amir Zamani.

