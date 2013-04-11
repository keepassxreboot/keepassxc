/**
 ***************************************************************************
 * @file Server.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "Server.h"
#include "qhttpserver/qhttpserver.h"
#include "qhttpserver/qhttprequest.h"
#include "qhttpserver/qhttpresponse.h"
#include "Protocol.h"
#include "crypto/Crypto.h"
#include <QtCore/QHash>
#include <QtCore/QCryptographicHash>
#include <QtGui/QMessageBox>

using namespace KeepassHttpProtocol;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Request
////////////////////////////////////////////////////////////////////////////////////////////////////

RequestHandler::RequestHandler(QHttpRequest *request, QHttpResponse *response): m_request(request), m_response(response)
{
    m_request->storeBody();
    connect(m_request, SIGNAL(end()), this, SLOT(onEnd()));
    connect(m_response, SIGNAL(done()), this, SLOT(deleteLater()));
}

RequestHandler::~RequestHandler()
{
    delete m_request;
}

void RequestHandler::onEnd()
{
    Q_EMIT requestComplete(m_request, m_response);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Request
////////////////////////////////////////////////////////////////////////////////////////////////////

Server::Server(QObject *parent) :
    QObject(parent),
    m_httpServer(new QHttpServer(this)),
    m_started(false)
{
    connect(m_httpServer, SIGNAL(newRequest(QHttpRequest*,QHttpResponse*)), this, SLOT(handleRequest(QHttpRequest*,QHttpResponse*)));
}

void Server::start()
{
    if (m_started)
        return;
    m_started = m_httpServer->listen(QHostAddress::LocalHost, 19455);
}

void Server::stop()
{
    if (!m_started)
        return;
    m_httpServer->close();
    m_started = false;
}

void Server::handleRequest(QHttpRequest *request, QHttpResponse *response)
{
    RequestHandler * h = new RequestHandler(request, response);
    connect(h, SIGNAL(requestComplete(QHttpRequest*,QHttpResponse*)), this, SLOT(handleRequestComplete(QHttpRequest*,QHttpResponse*)));
}

void Server::handleRequestComplete(QHttpRequest *request, QHttpResponse *response)
{
    Request r;
    if (!isDatabaseOpened() && !openDatabase()) {
        response->writeHead(QHttpResponse::STATUS_SERVICE_UNAVAILABLE);
        response->end();
    } else if (request->header("content-type").compare("application/json", Qt::CaseInsensitive) == 0 &&
               r.fromJson(request->body())) {

        QByteArray hash = QCryptographicHash::hash((getDatabaseRootUuid() + getDatabaseRecycleBinUuid()).toUtf8(),
                                                   QCryptographicHash::Sha1).toHex();

        Response protocolResp(r, QString::fromAscii(hash));
        switch(r.requestType()) {
        case INVALID:           break;
        case TEST_ASSOCIATE:    testAssociate(r, &protocolResp); break;
        case ASSOCIATE:         associate(r, &protocolResp); break;
        case GET_LOGINS:        getLogins(r, &protocolResp); break;
        case GET_LOGINS_COUNT:  getLoginsCount(r, &protocolResp); break;
        case GET_ALL_LOGINS:    getAllLogins(r, &protocolResp); break;
        case SET_LOGIN:         setLogin(r, &protocolResp); break;
        case GENERATE_PASSWORD: generatePassword(r, &protocolResp); break;
        }

        QByteArray s = protocolResp.toJson().toUtf8();
        response->setHeader("Content-Type", "application/json");
        response->setHeader("Content-Length", QString::number(s.size()));
        response->writeHead(QHttpResponse::STATUS_OK);
        response->write(s);
        response->end();
    } else {
        response->writeHead(QHttpResponse::STATUS_BAD_REQUEST);
        response->end();
    }
}

void Server::testAssociate(const Request& r, Response * protocolResp)
{
    if (r.id().isEmpty())
        return;   //ping

    QString key = getKey(r.id());
    if (key.isEmpty() || !r.CheckVerifier(key))
        return;

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
}

void Server::associate(const Request& r, Response * protocolResp)
{
    if (!r.CheckVerifier(r.key()))
        return;

    QString id = storeKey(r.key());
    if (id.isEmpty())
        return;

    protocolResp->setSuccess();
    protocolResp->setId(id);
    protocolResp->setVerifier(r.key());
}

void Server::getLogins(const Request &r, Response *protocolResp)
{
    QString key = getKey(r.id());
    if (!r.CheckVerifier(key))
        return;

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
    QList<Entry> entries = findMatchingEntries(r.id(), r.url(), r.submitUrl(), r.realm());  //TODO: filtering, request confirmation [in db adaptation layer?]
    if (r.sortSelection()) {
        //TODO: sorting (in db adaptation layer? here?)
    }
    protocolResp->setEntries(entries);
}

void Server::getLoginsCount(const Request &r, Response *protocolResp)
{
    QString key = getKey(r.id());
    if (!r.CheckVerifier(key))
        return;

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
    protocolResp->setCount(countMatchingEntries(r.id(), r.url(), r.submitUrl(), r.realm()));
}

void Server::getAllLogins(const Request &r, Response *protocolResp)
{
    QString key = getKey(r.id());
    if (!r.CheckVerifier(key))
        return;

//    parms.SearchString = @"^[A-Za-z0-9:/-]+\.[A-Za-z0-9:/-]+$"; // match anything looking like a domain or url
    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
    protocolResp->setEntries(searchAllEntries(r.id()));   //TODO: ensure there is no password --> change API?
}

void Server::setLogin(const Request &r, Response *protocolResp)
{
    QString key = getKey(r.id());
    if (!r.CheckVerifier(key))
        return;

    QString uuid = r.uuid();
    if (uuid.isEmpty())
        addEntry(r.id(), r.login(), r.password(), r.url(), r.submitUrl(), r.realm());
    else
        updateEntry(r.id(), r.uuid(), r.login(), r.password(), r.url());

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
}

void Server::generatePassword(const Request &r, Response *protocolResp)
{
    QString key = getKey(r.id());
    if (!r.CheckVerifier(key))
        return;

    QString password = generatePassword();

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
    protocolResp->setEntries(QList<Entry>() << Entry("generate-password", "generate-password", password, "generate-password"));

    memset(password.data(), 0, password.length());
}
