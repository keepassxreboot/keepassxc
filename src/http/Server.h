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
#include <microhttpd.h>


namespace KeepassHttpProtocol {

class Request;
class Response;
class Entry;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);

    //TODO: use QByteArray?
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

public Q_SLOTS:
    void start();
    void stop();

private Q_SLOTS:
    void handleRequest(const QByteArray in, QByteArray *out);
    void handleOpenDatabase(bool *success);

Q_SIGNALS:
    void emitRequest(const QByteArray in, QByteArray *out);
    void emitOpenDatabase(bool *success);
    void donewrk();

private:
    void testAssociate(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void associate(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getLogins(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getLoginsCount(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void getAllLogins(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void setLogin(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);
    void generatePassword(const KeepassHttpProtocol::Request &r, KeepassHttpProtocol::Response *protocolResp);

    static int request_handler_wrapper(void *me,
        struct MHD_Connection *connection,
        const char *url, const char *method, const char *version,
        const char *upload_data, size_t *upload_data_size, void **con_cls);
    static void request_completed(void *, struct MHD_Connection *,
        void **con_cls, enum MHD_RequestTerminationCode);

    int request_handler(struct MHD_Connection *connection,
        const char *, const char *method, const char *,
        const char *upload_data, size_t *upload_data_size, void **con_cls);
    int send_response(struct MHD_Connection *connection, const char *page);
    int send_unavailable(struct MHD_Connection *connection);

    bool m_started;
    struct MHD_Daemon *daemon;

    struct connection_info_struct {
        char *response;
    };
};

}   /*namespace KeepassHttpProtocol*/

#endif // SERVER_H
