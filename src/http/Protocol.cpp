/**
 ***************************************************************************
 * @file Response.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "Protocol.h"

#include <QtCore/QMetaProperty>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "qjson/parser.h"
#include "qjson/qobjecthelper.h"
#include "qjson/serializer.h"

#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"

namespace KeepassHttpProtocol
{
static const char * const STR_GET_LOGINS = "get-logins";
static const char * const STR_GET_LOGINS_COUNT = "get-logins-count";
static const char * const STR_GET_ALL_LOGINS = "get-all-logins";
static const char * const STR_SET_LOGIN = "set-login";
static const char * const STR_ASSOCIATE = "associate";
static const char * const STR_TEST_ASSOCIATE = "test-associate";
static const char * const STR_GENERATE_PASSWORD = "generate-password";
static const char * const STR_VERSION = "1.5.0.0";
}/*namespace KeepassHttpProtocol*/

using namespace KeepassHttpProtocol;

static QHash<QString, RequestType> createStringHash()
{
    QHash<QString, RequestType> hash;
    hash.insert(STR_GET_LOGINS,       GET_LOGINS);
    hash.insert(STR_GET_LOGINS_COUNT, GET_LOGINS_COUNT);
    hash.insert(STR_GET_ALL_LOGINS,   GET_ALL_LOGINS);
    hash.insert(STR_SET_LOGIN,        SET_LOGIN);
    hash.insert(STR_ASSOCIATE,        ASSOCIATE);
    hash.insert(STR_TEST_ASSOCIATE,   TEST_ASSOCIATE);
    hash.insert(STR_GENERATE_PASSWORD,GENERATE_PASSWORD);
    return hash;
}

static RequestType parseRequest(const QString &str)
{
    static const QHash<QString, RequestType> REQUEST_STRINGS = createStringHash();
    return REQUEST_STRINGS.value(str, INVALID);
}

static QByteArray decode64(QString s)
{
    return QByteArray::fromBase64(s.toAscii());
}

static QString encode64(QByteArray b)
{
    return QString::fromAscii(b.toBase64());
}

static QByteArray decrypt2(const QByteArray & data, SymmetricCipher & cipher)
{
    //Ensure we get full blocks only
    if (data.length() <= 0 || data.length() % cipher.blockSize())
        return QByteArray();

    //Decrypt
    cipher.reset();
    QByteArray buffer = cipher.process(data);

    //Remove PKCS#7 padding
    buffer.chop(buffer.at(buffer.length()-1));
    return buffer;
}

static QString decrypt(const QString &data, SymmetricCipher &cipher)
{
    return QString::fromUtf8(decrypt2(decode64(data), cipher));
}

static QByteArray encrypt2(const QByteArray & data, SymmetricCipher & cipher)
{
    //Add PKCS#7 padding
    const int blockSize = cipher.blockSize();
    const int paddingSize = blockSize - data.size() % blockSize;

    //Encrypt
    QByteArray buffer = data + QByteArray(paddingSize, paddingSize);
    cipher.reset();
    cipher.processInPlace(buffer);
    return buffer;
}

static QString encrypt(const QString & data, SymmetricCipher & cipher)
{
    return encode64(encrypt2(data.toUtf8(), cipher));
}

static QVariant qobject2qvariant(const QObject * object, const QStringList ignoredProperties = QStringList(QString(QLatin1String("objectName"))))
{
    QVariantMap result;
    const QMetaObject *metaobject = object->metaObject();
    int count = metaobject->propertyCount();
    for (int i=0; i<count; ++i) {
        QMetaProperty metaproperty = metaobject->property(i);
        const char *name = metaproperty.name();

        if (ignoredProperties.contains(QLatin1String(name)) || (!metaproperty.isReadable()))
            continue;

        QVariant value = object->property(name);
        if (!value.isNull() /*&& value.isValid()*/)   //Do not add NULL or invalid fields
            result[QLatin1String(name)] = value;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Request
////////////////////////////////////////////////////////////////////////////////////////////////////

Request::Request(): m_requestType(INVALID)
{
}

QString Request::nonce() const
{
    return m_nonce;
}

void Request::setNonce(const QString &nonce)
{
    m_nonce = nonce;
}

QString Request::verifier() const
{
    return m_verifier;
}

void Request::setVerifier(const QString &verifier)
{
    m_verifier = verifier;
}

QString Request::id() const
{
    return m_id;
}

void Request::setId(const QString &id)
{
    m_id = id;
}

QString Request::key() const
{
    return m_key;
}

void Request::setKey(const QString &key)
{
    m_key = key;
}

QString Request::submitUrl() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_submitUrl, m_cipher);
}

void Request::setSubmitUrl(const QString &submitUrl)
{
    m_submitUrl = submitUrl;
}

QString Request::url() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_url, m_cipher);
}

void Request::setUrl(const QString &url)
{
    m_url = url;
}

QString Request::realm() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_realm, m_cipher);
}

void Request::setRealm(const QString &realm)
{
    m_realm = realm;
}

QString Request::login() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_login, m_cipher);
}

void Request::setLogin(const QString &login)
{
    m_login = login;
}

QString Request::uuid() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_uuid, m_cipher);
}

void Request::setUuid(const QString &uuid)
{
    m_uuid = uuid;
}

QString Request::password() const
{
    Q_ASSERT(m_cipher.isValid());
    return decrypt(m_password, m_cipher);
}

void Request::setPassword(const QString &password)
{
    m_password = password;
}

bool Request::sortSelection() const
{
    return m_sortSelection;
}

void Request::setSortSelection(bool sortSelection)
{
    m_sortSelection = sortSelection;
}

KeepassHttpProtocol::RequestType Request::requestType() const
{
    return parseRequest(m_requestType);
}

QString Request::requestTypeStr() const
{
    return m_requestType;
}

void Request::setRequestType(const QString &requestType)
{
    m_requestType = requestType;
}

bool Request::CheckVerifier(const QString &key) const
{
    Q_ASSERT(!m_cipher.isValid());
    m_cipher.init(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Decrypt,
                  decode64(key), decode64(m_nonce));
    return decrypt(m_verifier, m_cipher) == m_nonce;
}

bool Request::fromJson(QString text)
{
    bool isok = false;
    QVariant v = QJson::Parser().parse(text.toUtf8(), &isok);
    if (!isok)
        return false;

    m_requestType.clear();
    QJson::QObjectHelper::qvariant2qobject(v.toMap(), this);

    return requestType() != INVALID;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Response
////////////////////////////////////////////////////////////////////////////////////////////////////

Response::Response(const Request &request, QString hash):
    m_requestType(request.requestTypeStr()),
    m_success(false),
    m_count(-1),
    m_version(STR_VERSION),
    m_hash(hash)
{
}

void Response::setVerifier(QString key)
{
    Q_ASSERT(!m_cipher.isValid());
    m_cipher.init(SymmetricCipher::Aes256, SymmetricCipher::Cbc, SymmetricCipher::Encrypt, decode64(key));

    //Generate new IV
    const QByteArray iv = Random::randomArray(m_cipher.blockSize());
    m_cipher.setIv(iv);
    m_nonce = encode64(iv);

    //Encrypt
    m_verifier = encrypt(m_nonce, m_cipher);
}

QString Response::toJson()
{
    QVariant result = qobject2qvariant(this);

    QJson::Serializer s;
    s.setIndentMode(QJson::IndentCompact);
    return s.serialize(result);
}

KeepassHttpProtocol::RequestType Response::requestType() const
{
    return parseRequest(m_requestType);
}

QString Response::requestTypeStr() const
{
    return m_requestType;
}

QString Response::verifier() const
{
    return m_verifier;
}

QString Response::nonce() const
{
    return m_nonce;
}

QVariant Response::count() const
{
    return m_count < 0 ? QVariant() : QVariant(m_count);
}

void Response::setCount(int count)
{
    m_count = count;
}

QVariant Response::getEntries() const
{
    if (m_count < 0 || m_entries.isEmpty())
        return QVariant();

    QList<QVariant> res;
    res.reserve(m_entries.size());
    Q_FOREACH(const Entry &entry, m_entries)
        res.append(qobject2qvariant(&entry));
    return res;
}

void Response::setEntries(const QList<Entry> &entries)
{
    Q_ASSERT(m_cipher.isValid());

    m_count = entries.count();

    QList<Entry> encryptedEntries;
    encryptedEntries.reserve(m_count);
    Q_FOREACH(const Entry &entry, entries) {
        encryptedEntries << Entry(encrypt(entry.name(), m_cipher),
                                  encrypt(entry.login(), m_cipher),
                                  entry.password().isNull() ? QString() : encrypt(entry.password(), m_cipher),
                                  encrypt(entry.uuid(), m_cipher));
    }
    m_entries = encryptedEntries;
}

QString Response::hash() const
{
    return m_hash;
}

QString Response::version() const
{
    return m_version;
}

QString Response::id() const
{
    return m_id;
}

void Response::setId(const QString &id)
{
    m_id = id;
}

bool Response::success() const
{
    return m_success;
}

void Response::setSuccess()
{
    m_success = true;
}

QString Response::error() const
{
    return m_error;
}

void Response::setError(const QString &error)
{
    m_success = false;
    m_error = error;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// ResponseEntry
////////////////////////////////////////////////////////////////////////////////////////////////////

Q_DECLARE_METATYPE(Entry)

Entry::Entry()
{}

Entry::Entry(QString name, QString login, QString password, QString uuid):
    m_login(login),
    m_password(password),
    m_uuid(uuid),
    m_name(name)
{}

Entry::Entry(const Entry & other):
    QObject(),
    m_login(other.m_login),
    m_password(other.m_password),
    m_uuid(other.m_uuid),
    m_name(other.m_name)
{}

Entry & Entry::operator=(const Entry & other)
{
    m_login = other.m_login;
    m_password = other.m_password;
    m_uuid = other.m_uuid;
    m_name = other.m_name;
    return *this;
}

QString Entry::login() const
{
    return m_login;
}

QString Entry::name() const
{
    return m_name;
}

QString Entry::uuid() const
{
    return m_uuid;
}

QString Entry::password() const
{
    return m_password;
}
