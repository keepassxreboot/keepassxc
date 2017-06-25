/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 or (at your option)
*  version 3 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QEventLoop>
#include <QtCore/QHash>
#include <QtCore/QCryptographicHash>
#include <QtWidgets/QMessageBox>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverresponse.hpp"
#include "qhttp/qhttpserverrequest.hpp"

#include "Server.h"
#include "Protocol.h"
#include "HttpSettings.h"
#include "crypto/Crypto.h"

using namespace KeepassHttpProtocol;
using namespace qhttp::server;

Server::Server(QObject *parent) :
    QObject(parent),
    m_started(false),
    m_server(nullptr)
{

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
    QString bits = QString::number(HttpSettings::getbits());

    protocolResp->setSuccess();
    protocolResp->setId(r.id());
    protocolResp->setVerifier(key);
    protocolResp->setEntries(QList<Entry>() << Entry("generate-password", bits, password, "generate-password"));

    memset(password.data(), 0, password.length());
}

void Server::handleRequest(const QByteArray& data, QHttpResponse* response)
{
    Request r;
    if (!r.fromJson(data))
        return;

    QByteArray hash = QCryptographicHash::hash(
        (getDatabaseRootUuid() + getDatabaseRecycleBinUuid()).toUtf8(),
         QCryptographicHash::Sha1).toHex();

    Response protocolResp(r, QString::fromLatin1(hash));
    switch(r.requestType()) {
    case INVALID: break;
    case GET_LOGINS:        getLogins(r, &protocolResp); break;
    case GET_LOGINS_COUNT:  getLoginsCount(r, &protocolResp); break;
    case GET_ALL_LOGINS:    getAllLogins(r, &protocolResp); break;
    case SET_LOGIN:         setLogin(r, &protocolResp); break;
    case ASSOCIATE:         associate(r, &protocolResp); break;
    case TEST_ASSOCIATE:    testAssociate(r, &protocolResp); break;
    case GENERATE_PASSWORD: generatePassword(r, &protocolResp); break;
    }

    QString out = protocolResp.toJson().toUtf8();

    // THIS IS A FAKE HACK!!!
    // the real "error" is a misbehavior in the QJSON qobject2qvariant method
    // 1. getLogins returns an empty QList<Entry> into protocolResp->m_entries
    // in toJson() function the following happend:
    // 2. QJson::QObjectHelper::qobject2qvariant marks m_entries as invalid !!!
    // 3. QJson::Serializer write out Entries as null instead of empty list
    //(4. ChromeIPass tries to access Entries.length and fails with null pointer exception)
    // the fake workaround replaces the (wrong) "Entries":null with "Entries:[] to give
    // chromeIPass (and passIFox) en empty list
    int pos1 = out.indexOf("\"Count\":0,");
    int pos2 = out.indexOf("\"Entries\":null,");
    if (pos1 != -1 && pos2 != -1) {
        out.replace(pos2, 15, "\"Entries\":[],");
    }

    response->setStatusCode(qhttp::ESTATUS_OK);
    response->addHeader("Content-Type", "application/json");
    response->end(out.toUtf8());
}

void Server::start(void)
{
    if (m_started)
        return;

    // local loopback hardcoded, since KeePassHTTP handshake
    // is not safe against interception
    QHostAddress address("127.0.0.1");
    int port = HttpSettings::httpPort();

    m_server = new QHttpServer(this);
    m_server->listen(address, port);
    connect(m_server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)), this, SLOT(onNewRequest(QHttpRequest*, QHttpResponse*)));

    m_started = true;
}

void Server::stop(void)
{
    if (!m_started)
        return;

    m_server->stopListening();
    m_server->deleteLater();
    m_started = false;
}

void Server::onNewRequest(QHttpRequest* request, QHttpResponse* response)
{
    if (!isDatabaseOpened()) {
        if (!openDatabase()) {
            response->setStatusCode(qhttp::ESTATUS_SERVICE_UNAVAILABLE);
            response->end();
            return;
        }
    }

    request->collectData(1024);

    request->onEnd([=]() {
        this->handleRequest(request->collectedData(), response);
    });
}
