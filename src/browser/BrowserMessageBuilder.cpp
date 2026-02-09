/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#ifdef QT_DEBUG
#include <QDebug>
#endif

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
                                                 const QString& nonce,
                                                 const Parameters& params,
                                                 const QString& publicKey,
                                                 const QString& secretKey)
{
    auto message = buildMessage(nonce);

    Parameters::const_iterator i;
    for (i = params.constBegin(); i != params.constEnd(); ++i) {
        message[i.key()] = QJsonValue::fromVariant(i.value());
    }

    const auto encryptedMessage = encryptMessage(message, nonce, publicKey, secretKey);
    if (encryptedMessage.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_ENCRYPT_MESSAGE);
    }

    QJsonObject response;
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
    case ERROR_KEEPASS_ACCESS_TO_ALL_ENTRIES_DENIED:
        return QObject::tr("Access to all entries is denied");
    case ERROR_PASSKEYS_ATTESTATION_NOT_SUPPORTED:
        return QObject::tr("Attestation not supported");
    case ERROR_PASSKEYS_CREDENTIAL_IS_EXCLUDED:
        return QObject::tr("Credential is excluded");
    case ERROR_PASSKEYS_REQUEST_CANCELED:
        return QObject::tr("Passkeys request canceled");
    case ERROR_PASSKEYS_INVALID_USER_VERIFICATION:
        return QObject::tr("Invalid user verification");
    case ERROR_PASSKEYS_EMPTY_PUBLIC_KEY:
        return QObject::tr("Empty public key");
    case ERROR_PASSKEYS_INVALID_URL_PROVIDED:
        return QObject::tr("Invalid URL provided");
    case ERROR_PASSKEYS_ORIGIN_NOT_ALLOWED:
        return QObject::tr("Origin is empty or not allowed");
    case ERROR_PASSKEYS_DOMAIN_IS_NOT_VALID:
        return QObject::tr("Effective domain is not a valid domain");
    case ERROR_PASSKEYS_DOMAIN_RPID_MISMATCH:
        return QObject::tr("Origin and RP ID do not match");
    case ERROR_PASSKEYS_NO_SUPPORTED_ALGORITHMS:
        return QObject::tr("No supported algorithms were provided");
    case ERROR_PASSKEYS_WAIT_FOR_LIFETIMER:
        return QObject::tr("Wait for timer to expire");
    case ERROR_PASSKEYS_UNKNOWN_ERROR:
        return QObject::tr("Unknown passkeys error");
    case ERROR_PASSKEYS_INVALID_CHALLENGE:
        return QObject::tr("Challenge is shorter than required minimum length");
    case ERROR_PASSKEYS_INVALID_USER_ID:
        return QObject::tr("user.id does not match the required length");
    case ERROR_KEEPASS_CANNOT_USE_REFERENCES:
        return QObject::tr("Username or password cannot contain references");
    case ERROR_PASSKEYS_EVAL_BY_CREDENTIAL_NOT_SUPPORTED:
        return QObject::tr("evalByCredential is not supported at registration");
    case ERROR_PASSKEYS_EVAL_BY_CREDENTIAL_NOT_EMPTY:
        return QObject::tr("evalByCredential is not empty, but allowedCredentials is");
    case ERROR_PASSKEYS_EVAL_BY_CREDENTIAL_NOT_FOUND:
        return QObject::tr("Credential ID provided in evalByCredential not found");
    default:
        return QObject::tr("Unknown error");
    }
}

QString BrowserMessageBuilder::encryptMessage(const QJsonObject& message,
                                              const QString& nonce,
                                              const QString& publicKey,
                                              const QString& secretKey,
                                              const qsizetype maxLength,
                                              const qsizetype macBytes)
{
    if (message.isEmpty() || nonce.isEmpty()) {
        return {};
    }

    const auto reply(QJsonDocument(message).toJson());
    if (!reply.isEmpty()) {
        return encrypt(reply, nonce, publicKey, secretKey, maxLength, macBytes);
    }

    return {};
}

QJsonObject BrowserMessageBuilder::decryptMessage(const QString& message,
                                                  const QString& nonce,
                                                  const QString& publicKey,
                                                  const QString& secretKey,
                                                  const qsizetype maxLength,
                                                  const qsizetype macBytes)
{
    if (message.isEmpty() || nonce.isEmpty()) {
        return {};
    }

    const auto ba = decrypt(message, nonce, publicKey, secretKey, maxLength, macBytes);
    if (ba.isEmpty()) {
        return {};
    }

    return getJsonObject(ba);
}

QString BrowserMessageBuilder::encrypt(const QString& plaintext,
                                       const QString& nonce,
                                       const QString& publicKey,
                                       const QString& secretKey,
                                       const qsizetype maxLength,
                                       const qsizetype macBytes)
{
    const QByteArray messageBytes = plaintext.toUtf8();
    if (messageBytes.length() > maxLength) {
        qWarning() << "Message length" << messageBytes.length() << "exceeds the maximum size.";
        return {};
    }

    const QByteArray nonceBytes = base64Decode(nonce);
    if (nonceBytes.length() != crypto_box_NONCEBYTES) {
        qWarning() << "Nonce length" << nonceBytes.length() << "is invalid.";
        return {};
    }

    const QByteArray publicKeyBytes = base64Decode(publicKey);
    if (publicKeyBytes.length() != crypto_box_PUBLICKEYBYTES) {
        qWarning() << "Public key length" << publicKeyBytes.length() << "is invalid.";
        return {};
    }

    const QByteArray secretKeyBytes = base64Decode(secretKey);
    if (secretKeyBytes.length() != crypto_box_SECRETKEYBYTES) {
        qWarning() << "Secret key length" << secretKeyBytes.length() << "is invalid.";
        return {};
    }

    const std::vector<unsigned char> messageVec(messageBytes.cbegin(), messageBytes.cend());
    const std::vector<unsigned char> nonceVec(nonceBytes.cbegin(), nonceBytes.cend());
    const std::vector<unsigned char> publicKeyVec(publicKeyBytes.cbegin(), publicKeyBytes.cend());
    const std::vector<unsigned char> secretKeyVec(secretKeyBytes.cbegin(), secretKeyBytes.cend());
    if (messageVec.empty() || nonceVec.empty() || publicKeyVec.empty() || secretKeyVec.empty()) {
        return {};
    }

    std::vector<unsigned char> encryptedData;
    encryptedData.resize(maxLength + macBytes);

    if (crypto_box_easy(encryptedData.data(),
                        messageVec.data(),
                        messageVec.size(),
                        nonceVec.data(),
                        publicKeyVec.data(),
                        secretKeyVec.data())
        == 0) {
        const auto res = getQByteArray(encryptedData.data(), (macBytes + messageBytes.length()));
        const auto resBase64 = res.toBase64();
        if (resBase64.length() > BrowserShared::SOCKET_BUFFER_SIZE) {
            qWarning() << "Encoded message length" << resBase64.length() << "exceeds the maximum socket buffer size.";
            return {};
        }
        return resBase64;
    }

    return {};
}

QByteArray BrowserMessageBuilder::decrypt(const QString& encrypted,
                                          const QString& nonce,
                                          const QString& publicKey,
                                          const QString& secretKey,
                                          const qsizetype maxLength,
                                          const qsizetype macBytes)
{
    const QByteArray encryptedBytes = base64Decode(encrypted);
    if (encryptedBytes.size() == 0 && encrypted.size() > 0) {
        qWarning() << "Message is not a valid base64 encoded string.";
    }
    if (maxLength < 0 || encryptedBytes.size() < macBytes) {
        qWarning() << "Message length" << encryptedBytes.length() << "is smaller than required.";
        return {};
    }
    if (encryptedBytes.size() - macBytes > maxLength) {
        qWarning() << "Message length" << encryptedBytes.length() << "exceeds the maximum size.";
        return {};
    }

    const QByteArray nonceBytes = base64Decode(nonce);
    if (nonceBytes.length() != crypto_box_NONCEBYTES) {
        qWarning() << "Nonce length" << nonceBytes.length() << "is invalid.";
        return {};
    }

    const QByteArray publicKeyBytes = base64Decode(publicKey);
    if (publicKeyBytes.length() != crypto_box_PUBLICKEYBYTES) {
        qWarning() << "Public key length" << publicKeyBytes.length() << "is invalid.";
        return {};
    }

    const QByteArray secretKeyBytes = base64Decode(secretKey);
    if (secretKeyBytes.length() != crypto_box_SECRETKEYBYTES) {
        qWarning() << "Secret key length" << secretKeyBytes.length() << "is invalid.";
        return {};
    }

    const std::vector<unsigned char> encryptedVec(encryptedBytes.cbegin(), encryptedBytes.cend());
    const std::vector<unsigned char> nonceVec(nonceBytes.cbegin(), nonceBytes.cend());
    const std::vector<unsigned char> publicKeyVec(publicKeyBytes.cbegin(), publicKeyBytes.cend());
    const std::vector<unsigned char> secretKeyVec(secretKeyBytes.cbegin(), secretKeyBytes.cend());
    if (encryptedVec.empty() || nonceVec.empty() || publicKeyVec.empty() || secretKeyVec.empty()) {
        return {};
    }

    std::vector<unsigned char> decryptedData;
    decryptedData.resize(encryptedVec.size() - macBytes);

    if (crypto_box_open_easy(decryptedData.data(),
                             encryptedVec.data(),
                             encryptedBytes.length(),
                             nonceVec.data(),
                             publicKeyVec.data(),
                             secretKeyVec.data())
        == 0) {
        return getQByteArray(decryptedData.data(), decryptedData.size());
    }

    return {};
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
#ifdef QT_DEBUG
    if (doc.isNull()) {
        qWarning() << "Cannot create QJsonDocument: " << err.errorString();
    }
#endif
    return doc.object();
}

QJsonObject BrowserMessageBuilder::getJsonObject(const QByteArray& ba) const
{
    QJsonParseError err;
    QJsonDocument doc(QJsonDocument::fromJson(ba, &err));
#ifdef QT_DEBUG
    if (doc.isNull()) {
        qWarning() << "Cannot create QJsonDocument: " << err.errorString();
    }
#endif

    return doc.object();
}

QByteArray BrowserMessageBuilder::base64Decode(const QString& str)
{
    // Returns an empty QByteArray if the string is not valid base64
    return QByteArray::fromBase64(str.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
}

QString BrowserMessageBuilder::incrementNonce(const QString& nonce)
{
    const QByteArray nonceArray = base64Decode(nonce);
    if (nonceArray.length() != crypto_box_NONCEBYTES) {
        qWarning() << "Nonce length" << nonceArray.length() << "is invalid.";
        return {};
    }

    std::vector<unsigned char> n(nonceArray.cbegin(), nonceArray.cend());

    sodium_increment(n.data(), n.size());
    return getQByteArray(n.data(), n.size()).toBase64();
}

QString BrowserMessageBuilder::getRandomBytesAsBase64(int bytes) const
{
    if (bytes == 0) {
        return {};
    }

    std::shared_ptr<unsigned char[]> buf(new unsigned char[bytes]);
    Botan::Sodium::randombytes_buf(buf.get(), bytes);

    return getBase64FromArray(reinterpret_cast<const char*>(buf.get()), bytes);
}

QString BrowserMessageBuilder::getBase64FromArray(const char* arr, int len) const
{
    if (len < 1) {
        return {};
    }

    auto data = QByteArray::fromRawData(arr, len);
    return getBase64FromArray(data);
}

// Returns URL encoded base64 with trailing removed
QString BrowserMessageBuilder::getBase64FromArray(const QByteArray& byteArray) const
{
    if (byteArray.length() < 1) {
        return {};
    }

    return byteArray.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString BrowserMessageBuilder::getBase64FromJson(const QJsonObject& jsonObject) const
{
    if (jsonObject.isEmpty()) {
        return {};
    }

    const auto dataArray = QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    return getBase64FromArray(dataArray);
}

QByteArray BrowserMessageBuilder::getArrayFromHexString(const QString& hexString) const
{
    return QByteArray::fromHex(hexString.toUtf8());
}

QByteArray BrowserMessageBuilder::getArrayFromBase64(const QString& base64str) const
{
    return QByteArray::fromBase64(base64str.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QByteArray BrowserMessageBuilder::getSha256Hash(const QString& str) const
{
    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Sha256);
}

QString BrowserMessageBuilder::getSha256HashAsBase64(const QString& str) const
{
    return getBase64FromArray(QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Sha256));
}
