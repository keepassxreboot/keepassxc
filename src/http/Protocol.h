/**
 ***************************************************************************
 * @file Response.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef RESPONSE_H
#define RESPONSE_H

#include <QtCore/QObject>
#include <QtCore/QCryptographicHash>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>
#include "crypto/SymmetricCipher.h"

namespace KeepassHttpProtocol {

enum RequestType {
    INVALID = -1,
    GET_LOGINS,
    GET_LOGINS_COUNT,
    GET_ALL_LOGINS,
    SET_LOGIN,
    ASSOCIATE,
    TEST_ASSOCIATE,
    GENERATE_PASSWORD
};

//TODO: use QByteArray whenever possible?

class Request : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString RequestType   READ requestTypeStr WRITE setRequestType  )
    Q_PROPERTY(bool    SortSelection READ sortSelection  WRITE setSortSelection)
    Q_PROPERTY(QString Login         READ login          WRITE setLogin        )
    Q_PROPERTY(QString Password      READ password       WRITE setPassword     )
    Q_PROPERTY(QString Uuid          READ uuid           WRITE setUuid         )
    Q_PROPERTY(QString Url           READ url            WRITE setUrl          )
    Q_PROPERTY(QString SubmitUrl     READ submitUrl      WRITE setSubmitUrl    )
    Q_PROPERTY(QString Key           READ key            WRITE setKey          )
    Q_PROPERTY(QString Id            READ id             WRITE setId           )
    Q_PROPERTY(QString Verifier      READ verifier       WRITE setVerifier     )
    Q_PROPERTY(QString Nonce         READ nonce          WRITE setNonce        )
    Q_PROPERTY(QString Realm         READ realm          WRITE setRealm        )

public:
    Request();
    bool fromJson(QString text);

    KeepassHttpProtocol::RequestType requestType() const;
    QString requestTypeStr() const;
    bool sortSelection() const;
    QString login() const;
    QString password() const;
    QString uuid() const;
    QString url() const;
    QString submitUrl() const;
    QString key() const;
    QString id() const;
    QString verifier() const;
    QString nonce() const;
    QString realm() const;
    bool CheckVerifier(const QString & key) const;

private:
    void setRequestType(const QString &requestType);
    void setSortSelection(bool sortSelection);
    void setLogin(const QString &login);
    void setPassword(const QString &password);
    void setUuid(const QString &uuid);
    void setUrl(const QString &url);
    void setSubmitUrl(const QString &submitUrl);
    void setKey(const QString &key);
    void setId(const QString &id);
    void setVerifier(const QString &verifier);
    void setNonce(const QString &nonce);
    void setRealm(const QString &realm);

    QString m_requestType;
    bool m_sortSelection;
    QString m_login;
    QString m_password;
    QString m_uuid;
    QString m_url;
    QString m_submitUrl;
    QString m_key;
    QString m_id;
    QString m_verifier;
    QString m_nonce;
    QString m_realm;
    mutable SymmetricCipher m_cipher;
};

class StringField : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString Key   READ key  )
    Q_PROPERTY(QString Value READ value)

public:
    StringField();
    StringField(const QString& key, const QString& value);
    StringField(const StringField & other);
    StringField &operator =(const StringField &other);

    QString key() const;
    QString value() const;

private:
    QString m_key;
    QString m_value;
};

class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString Login         READ login          )
    Q_PROPERTY(QString Password      READ password       )
    Q_PROPERTY(QString Uuid          READ uuid           )
    Q_PROPERTY(QString Name          READ name           )
    Q_PROPERTY(QVariant StringFields READ getStringFields)

public:
    Entry();
    Entry(QString name, QString login, QString password, QString uuid);
    Entry(const Entry & other);
    Entry &operator =(const Entry &other);

    QString login() const;
    QString password() const;
    QString uuid() const;
    QString name() const;
    QList<StringField> stringFields() const;
    void addStringField(const QString& key, const QString& value);

private:
    QVariant getStringFields() const;

    QString m_login;
    QString m_password;
    QString m_uuid;
    QString m_name;
    QList<StringField> m_stringFields;
};

class Response : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString RequestType READ requestTypeStr)
    Q_PROPERTY(QString Error       READ error         )
    Q_PROPERTY(bool    Success     READ success       )
    Q_PROPERTY(QString Id          READ id            )
    Q_PROPERTY(QString Version     READ version       )
    Q_PROPERTY(QString Hash        READ hash          )
    Q_PROPERTY(QVariant Count      READ count         )
    Q_PROPERTY(QVariant Entries    READ getEntries    )
    Q_PROPERTY(QString Nonce       READ nonce         )
    Q_PROPERTY(QString Verifier    READ verifier      )

public:
    Response(const Request &request, QString hash);

    KeepassHttpProtocol::RequestType requestType() const;
    QString error() const;
    void setError(const QString &error = QString());
    bool success() const;
    void setSuccess();
    QString id() const;
    void setId(const QString &id);
    QString version() const;
    QString hash() const;
    QVariant count() const;
    void setCount(int count);
    QVariant getEntries() const;
    void setEntries(const QList<Entry> &entries);
    QString nonce() const;
    QString verifier() const;
    void setVerifier(QString key);

    QString toJson();

private:
    QString requestTypeStr() const;

    QString m_requestType;
    QString m_error;
    bool    m_success;
    QString m_id;
    int     m_count;
    QString m_version;
    QString m_hash;
    QList<Entry> m_entries;
    QString m_nonce;
    QString m_verifier;
    SymmetricCipher m_cipher;
};

}/*namespace KeepassHttpProtocol*/

#endif // RESPONSE_H
