/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BrowserMessageBuilder.h"
#include "BrowserShared.h"
#include "config-keepassx.h"
#include "core/Global.h"
#include "core/Tools.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <botan/sodium.h>

using namespace Botan::Sodium;

Q_GLOBAL_STATIC(BrowserMessageBuilder, s_browserMessageBuilder);

BrowserMessageBuilder* BrowserMessageBuilder::instance()
{
    return s_browserMessageBuilder;
}

QPair<QString, QString> BrowserMessageBuilder::getKeyPair()
{
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);

    const QString publicKey = getBase64FromKey(pk, crypto_box_PUBLICKEYBYTES);
    const QString secretKey = getBase64FromKey(sk, crypto_box_SECRETKEYBYTES);
    return qMakePair(publicKey, secretKey);
}

QJsonObject BrowserMessageBuilder::getErrorReply(const QString& action, const int errorCode) const
{
    QJsonObject response;
    response["action"] = action;
    response["errorCode"] = QString::number(errorCode);
    response["error"] = getErrorMessage(errorCode);
    return response;
}

QJsonObject BrowserMessageBuilder::buildMessage(const QString& nonce) const
{
    QJsonObject message;
    message["version"] = KEEPASSXC_VERSION;
    message["success"] = TRUE_STR;
    message["nonce"] = nonce;
    return message;
}

QJsonObject BrowserMessageBuilder::buildResponse(const QString& action,
                                                 const QJsonObject& message,
                                                 const QString& nonce,
                                                 const QString& publicKey,
                                                 const QString& secretKey)
{
    QJsonObject response;
    QString encryptedMessage = encryptMessage(message, nonce, publicKey, secretKey);
    if (encryptedMessage.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_ENCRYPT_MESSAGE);
    }

    response["action"] = action;
    response["message"] = encryptedMessage;
    response["nonce"] = nonce;
    return response;
}

QString BrowserMessageBuilder::getErrorMessage(const int errorCode) const
{
    switch (errorCode) {
    case ERROR_KEEPASS_DATABASE_NOT_OPENED:
        return QObject::tr("Database not opened");
    case ERROR_KEEPASS_DATABASE_HASH_NOT_RECEIVED:
        return QObject::tr("Database hash not available");
    case ERROR_KEEPASS_CLIENT_PUBLIC_KEY_NOT_RECEIVED:
        return QObject::tr("Client public key not received");
    case ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE:
        return QObject::tr("Cannot decrypt message");
    case ERROR_KEEPASS_ACTION_CANCELLED_OR_DENIED:
        return QObject::tr("Action cancelled or denied");
    case ERROR_KEEPASS_CANNOT_ENCRYPT_MESSAGE:
        return QObject::tr("Message encryption failed.");
    case ERROR_KEEPASS_ASSOCIATION_FAILED:
        return QObject::tr("KeePassXC association failed, try again");
    case ERROR_KEEPASS_ENCRYPTION_KEY_UNRECOGNIZED:
        return QObject::tr("Encryption key is not recognized");
    case ERROR_KEEPASS_INCORRECT_ACTION:
        return QObject::tr("Incorrect action");
    case ERROR_KEEPASS_EMPTY_MESSAGE_RECEIVED:
        return QObject::tr("Empty message received");
    case ERROR_KEEPASS_NO_URL_PROVIDED:
        return QObject::tr("No URL provided");
    case ERROR_KEEPASS_NO_LOGINS_FOUND:
        return QObject::tr("No logins found");
    case ERROR_KEEPASS_NO_GROUPS_FOUND:
        return QObject::tr("No groups found");
    case ERROR_KEEPASS_CANNOT_CREATE_NEW_GROUP:
        return QObject::tr("Cannot create new group");
    case ERROR_KEEPASS_NO_VALID_UUID_PROVIDED:
        return QObject::tr("No valid UUID provided");
    default:
        return QObject::tr("Unknown error");
    }
}

QString BrowserMessageBuilder::encryptMessage(const QJsonObject& message,
                                              const QString& nonce,
                                              const QString& publicKey,
                                              const QString& secretKey)
{
    if (message.isEmpty() || nonce.isEmpty()) {
        return QString();
    }

    const QString reply(QJsonDocument(message).toJson());
    if (!reply.isEmpty()) {
        return encrypt(reply, nonce, publicKey, secretKey);
    }

    return QString();
}

QJsonObject BrowserMessageBuilder::decryptMessage(const QString& message,
                                                  const QString& nonce,
                                                  const QString& publicKey,
                                                  const QString& secretKey)
{
    if (message.isEmpty() || nonce.isEmpty()) {
        return QJsonObject();
    }

    QByteArray ba = decrypt(message, nonce, publicKey, secretKey);
    if (ba.isEmpty()) {
        return QJsonObject();
    }

    return getJsonObject(ba);
}

QString BrowserMessageBuilder::encrypt(const QString& plaintext,
                                       const QString& nonce,
                                       const QString& publicKey,
                                       const QString& secretKey)
{
    const QByteArray ma = plaintext.toUtf8();
    const QByteArray na = base64Decode(nonce);
    const QByteArray ca = base64Decode(publicKey);
    const QByteArray sa = base64Decode(secretKey);

    std::vector<unsigned char> m(ma.cbegin(), ma.cend());
    std::vector<unsigned char> n(na.cbegin(), na.cend());
    std::vector<unsigned char> ck(ca.cbegin(), ca.cend());
    std::vector<unsigned char> sk(sa.cbegin(), sa.cend());

    std::vector<unsigned char> e;
    e.resize(BrowserShared::NATIVEMSG_MAX_LENGTH);

    if (m.empty() || n.empty() || ck.empty() || sk.empty()) {
        return QString();
    }

    if (crypto_box_easy(e.data(), m.data(), m.size(), n.data(), ck.data(), sk.data()) == 0) {
        QByteArray res = getQByteArray(e.data(), (crypto_box_MACBYTES + ma.length()));
        return res.toBase64();
    }

    return QString();
}

QByteArray BrowserMessageBuilder::decrypt(const QString& encrypted,
                                          const QString& nonce,
                                          const QString& publicKey,
                                          const QString& secretKey)
{
    const QByteArray ma = base64Decode(encrypted);
    const QByteArray na = base64Decode(nonce);
    const QByteArray ca = base64Decode(publicKey);
    const QByteArray sa = base64Decode(secretKey);

    std::vector<unsigned char> m(ma.cbegin(), ma.cend());
    std::vector<unsigned char> n(na.cbegin(), na.cend());
    std::vector<unsigned char> ck(ca.cbegin(), ca.cend());
    std::vector<unsigned char> sk(sa.cbegin(), sa.cend());

    std::vector<unsigned char> d;
    d.resize(BrowserShared::NATIVEMSG_MAX_LENGTH);

    if (m.empty() || n.empty() || ck.empty() || sk.empty()) {
        return QByteArray();
    }

    if (crypto_box_open_easy(d.data(), m.data(), ma.length(), n.data(), ck.data(), sk.data()) == 0) {
        return getQByteArray(d.data(), std::char_traits<char>::length(reinterpret_cast<const char*>(d.data())));
    }

    return QByteArray();
}

QString BrowserMessageBuilder::getBase64FromKey(const uchar* array, const uint len)
{
    return getQByteArray(array, len).toBase64();
}

QByteArray BrowserMessageBuilder::getQByteArray(const uchar* array, const uint len) const
{
    QByteArray qba;
    qba.reserve(len);
    for (uint i = 0; i < len; ++i) {
        qba.append(static_cast<char>(array[i]));
    }
    return qba;
}

QJsonObject BrowserMessageBuilder::getJsonObject(const uchar* pArray, const uint len) const
{
    QByteArray arr = getQByteArray(pArray, len);
    QJsonParseError err;
    QJsonDocument doc(QJsonDocument::fromJson(arr, &err));
    return doc.object();
}

QJsonObject BrowserMessageBuilder::getJsonObject(const QByteArray& ba) const
{
    QJsonParseError err;
    QJsonDocument doc(QJsonDocument::fromJson(ba, &err));
    return doc.object();
}

QByteArray BrowserMessageBuilder::base64Decode(const QString& str)
{
    return QByteArray::fromBase64(str.toUtf8());
}

QString BrowserMessageBuilder::incrementNonce(const QString& nonce)
{
    const QByteArray nonceArray = base64Decode(nonce);
    std::vector<unsigned char> n(nonceArray.cbegin(), nonceArray.cend());

    sodium_increment(n.data(), n.size());
    return getQByteArray(n.data(), n.size()).toBase64();
}
