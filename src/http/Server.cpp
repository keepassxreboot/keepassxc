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
#include <microhttpd.h>
#include "Protocol.h"
#include "HttpSettings.h"
#include "crypto/Crypto.h"
#include <QtCore/QHash>
#include <QtCore/QCryptographicHash>
#include <QtWidgets/QMessageBox>
#include <QEventLoop>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QHostAddress>

#ifdef Q_OS_WIN
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

using namespace KeepassHttpProtocol;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Request
////////////////////////////////////////////////////////////////////////////////////////////////////

Server::Server(QObject *parent) :
    QObject(parent),
    m_started(false)
{
    connect(this, SIGNAL(emitRequest(const QByteArray, QByteArray*)),
            this, SLOT(handleRequest(const QByteArray, QByteArray*)));
    connect(this, SIGNAL(emitOpenDatabase(bool*)),
            this, SLOT(handleOpenDatabase(bool*)));
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


int Server::send_response(struct MHD_Connection *connection, const char *page)
{
    int ret;
    struct MHD_Response *response;

    response = MHD_create_response_from_buffer(
            strlen(page), static_cast<void*>(const_cast<char*>(page)),
            MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;

    MHD_add_response_header (response, "Content-Type", "application/json");
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}


int Server::send_unavailable(struct MHD_Connection *connection)
{
    int ret;
    struct MHD_Response *response;

    response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    if (!response) return MHD_NO;

    ret = MHD_queue_response (connection, MHD_HTTP_SERVICE_UNAVAILABLE, response);
    MHD_destroy_response (response);

    return ret;
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


int Server::request_handler_wrapper(void *me, struct MHD_Connection *connection,
        const char *url, const char *method, const char *version,
        const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    Server *myself = static_cast<Server*>(me);

    if (myself)
        return myself->request_handler(connection, url, method, version,
                                       upload_data, upload_data_size, con_cls);
    else
        return MHD_NO;
}


void Server::handleRequest(const QByteArray in, QByteArray *out)
{
    *out = QByteArray();

    Request r;
    if (!r.fromJson(in))
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

    *out = protocolResp.toJson().toUtf8();

    // THIS IS A FAKE HACK!!!
    // the real "error" is a misbehavior in the QJSON qobject2qvariant method
    // 1. getLogins returns an empty QList<Entry> into protocolResp->m_entries
    // in toJson() function the following happend:
    // 2. QJson::QObjectHelper::qobject2qvariant marks m_entries as invalid !!!
    // 3. QJson::Serializer write out Entries as null instead of empty list
    //(4. ChromeIPass tries to access Entries.length and fails with null pointer exception)
    // the fake workaround replaces the (wrong) "Entries":null with "Entries:[] to give
    // chromeIPass (and passIFox) en empty list
    QString tmp_out = QString(*out);
    int tmp_pos1 = tmp_out.indexOf("\"Count\":0,");
    int tmp_pos2 = tmp_out.indexOf("\"Entries\":null,");
    if (tmp_pos1 != -1 && tmp_pos2 != -1) {
        tmp_out.replace(tmp_pos2, 15, "\"Entries\":[],");
    }
    *out = tmp_out.toUtf8();

    Q_EMIT donewrk();
}


void Server::handleOpenDatabase(bool *success)
{
    *success = openDatabase();
    Q_EMIT donewrk();
}


int Server::request_handler(struct MHD_Connection *connection,
        const char *, const char *method, const char *,
        const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    struct Server::connection_info_struct *con_info =
        static_cast<struct Server::connection_info_struct*>(*con_cls);

    if (!isDatabaseOpened()) {
        bool success;
        QEventLoop loop1;
        loop1.connect(this, SIGNAL(donewrk()), SLOT(quit()));
        Q_EMIT emitOpenDatabase(&success);
        loop1.exec();

        if (!success)
            return send_unavailable(connection);
    }

    if (con_info == NULL) {
        *con_cls = calloc(1, sizeof(*con_info));
        return MHD_YES;
    }

    if (strcmp (method, MHD_HTTP_METHOD_POST) != 0)
        return MHD_NO;

    if (*upload_data_size == 0) {
        if (con_info && con_info->response)
            return send_response(connection, con_info->response);
        else
            return MHD_NO;
    }

    QString type = MHD_lookup_connection_value(connection,
                                               MHD_HEADER_KIND, "Content-Type");
    if (!type.contains("application/json", Qt::CaseInsensitive))
        return MHD_NO;

    // Now process the POST request

    QByteArray post = QByteArray(upload_data, *upload_data_size);

    QByteArray s;
    QEventLoop loop;
    loop.connect(this, SIGNAL(donewrk()), SLOT(quit()));
    Q_EMIT emitRequest(post, &s);
    loop.exec();

    if (s.size() == 0)
        return MHD_NO;

    con_info->response = static_cast<char*>(calloc(1, s.size()+1));
    memcpy(con_info->response, s.data(), s.size());

    *upload_data_size = 0;

    return MHD_YES;
}


void Server::request_completed(void *, struct MHD_Connection *,
        void **con_cls, enum MHD_RequestTerminationCode)
{
    struct Server::connection_info_struct *con_info =
        static_cast<struct Server::connection_info_struct*>(*con_cls);

    if (con_info == NULL)
        return;

    if (con_info->response) free(con_info->response);
    free(con_info);
    *con_cls = NULL;
}


void Server::start(void)
{
    if (m_started)
        return;

    bool nohost = true;
    int port = HttpSettings::httpPort();

    QHostInfo info = QHostInfo::fromName(HttpSettings::httpHost());
    if (!info.addresses().isEmpty()) {
        void* addrx = NULL;
        unsigned int flags = MHD_USE_SELECT_INTERNALLY;
        QHostAddress address = info.addresses().first();

        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            struct sockaddr_in *addr = static_cast<struct sockaddr_in*>(calloc(1, sizeof(struct sockaddr_in)));
            addrx = static_cast<void*>(addr);
            addr->sin_family = AF_INET;
            addr->sin_port = htons(HttpSettings::httpPort());
            addr->sin_addr.s_addr = htonl(address.toIPv4Address());
            nohost = false;
        } else {
            struct sockaddr_in6 *addr = static_cast<struct sockaddr_in6*>(calloc(1, sizeof(struct sockaddr_in6)));
            addrx = static_cast<void*>(addr);
            addr->sin6_family = AF_INET6;
            addr->sin6_port = htons(HttpSettings::httpPort());
            memcpy(&addr->sin6_addr, address.toIPv6Address().c, 16);
            nohost = false;
            flags |= MHD_USE_IPv6;
        }

        if (nohost) {
            qWarning("HTTPPlugin: Faled to get configured host!");
        } else {
            if (NULL == (daemon = MHD_start_daemon(flags, port, NULL, NULL,
                                                   &this->request_handler_wrapper, this,
                                                   MHD_OPTION_NOTIFY_COMPLETED,
                                                   this->request_completed, NULL,
                                                   MHD_OPTION_SOCK_ADDR,
                                                   addrx,
                                                   MHD_OPTION_END))) {
                nohost = true;
                qWarning("HTTPPlugin: Failed to bind to configured host!");
            } else {
                nohost = false;
                //qWarning("HTTPPlugin: Binded to configured host.");
            }

        }

        if (addrx != NULL)
            free(addrx);
    }

    if (nohost) {
        if (NULL == (daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
                                 &this->request_handler_wrapper, this,
                                 MHD_OPTION_NOTIFY_COMPLETED,
                                 this->request_completed, NULL,
                                 MHD_OPTION_END))) {
            qWarning("HTTPPlugin: Fatal! Failed to bind to both configured and default hosts!");
        } else {
            qWarning("HTTPPlugin: Bound to fallback address 0.0.0.0/:::!");
        }
    }

    m_started = true;
}


void Server::stop(void)
{
    if (!m_started)
        return;

    MHD_stop_daemon(daemon);
    m_started = false;
}
