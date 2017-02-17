/**
 ***************************************************************************
 * @file Server.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef SERVER_H
#define SERVER_H

#include <QtCore/QObject>
#include <QtCore/QList>

namespace qhttp {
    namespace server {
        class QHttpServer;
        class QHttpRequest;
        class QHttpResponse;
    }
}

namespace KeepassHttpProtocol {

using namespace qhttp::server;

class Request;
class Response;
class Entry;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);

    virtual bool isDatabaseOpened() const = 0;
    virtual bool openDatabase() = 0;
    virtual QString getDatabaseRootUuid() = 0;
    virtual QString getDatabaseRecycleBinUuid() = 0;
    virtual QString getKey(const QString &id) = 0;
    virtual QString storeKey(const QString &key) = 0;
    virtual QList<Entry> findMatchingEntries(const QString &id, const QString &url, const QString & submitUrl, const QString & realm) = 0;
    virtual int countMatchingEntries(const QString &id, const QString &url, const QString & submitUrl, const QString & realm) = 0;
    virtual QList<Entry> searchAllEntries(const QString &id) = 0;
    virtual void addEntry(const QString &id, const QString &login, const QString &password, const QString &url, const QString &submitUrl, const QString &realm) = 0;
    virtual void updateEntry(const QString &id, const QString &uuid, const QString &login, const QString &password, const QString &url) = 0;
    virtual QString generatePassword() = 0;

public slots:
    void start();
    void stop();

private slots:
    void onNewRequest(QHttpRequest* request, QHttpResponse* response);
    void handleRequest(const QByteArray& data, QHttpResponse* response);

private:
    void testAssociate(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void associate(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getLogins(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getLoginsCount(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getAllLogins(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void setLogin(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void generatePassword(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);

    bool m_started;

    QHttpServer* m_server;
};

}   /*namespace KeepassHttpProtocol*/

#endif // SERVER_H
