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

#ifndef BROWSERMESSAGEBUILDER_H
#define BROWSERMESSAGEBUILDER_H

#include <QPair>
#include <QString>

class QJsonObject;

namespace
{
    enum
    {
        ERROR_KEEPASS_DATABASE_NOT_OPENED = 1,
        ERROR_KEEPASS_DATABASE_HASH_NOT_RECEIVED = 2,
        ERROR_KEEPASS_CLIENT_PUBLIC_KEY_NOT_RECEIVED = 3,
        ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE = 4,
        ERROR_KEEPASS_TIMEOUT_OR_NOT_CONNECTED = 5,
        ERROR_KEEPASS_ACTION_CANCELLED_OR_DENIED = 6,
        ERROR_KEEPASS_CANNOT_ENCRYPT_MESSAGE = 7,
        ERROR_KEEPASS_ASSOCIATION_FAILED = 8,
        ERROR_KEEPASS_KEY_CHANGE_FAILED = 9,
        ERROR_KEEPASS_ENCRYPTION_KEY_UNRECOGNIZED = 10,
        ERROR_KEEPASS_NO_SAVED_DATABASES_FOUND = 11,
        ERROR_KEEPASS_INCORRECT_ACTION = 12,
        ERROR_KEEPASS_EMPTY_MESSAGE_RECEIVED = 13,
        ERROR_KEEPASS_NO_URL_PROVIDED = 14,
        ERROR_KEEPASS_NO_LOGINS_FOUND = 15,
        ERROR_KEEPASS_NO_GROUPS_FOUND = 16,
        ERROR_KEEPASS_CANNOT_CREATE_NEW_GROUP = 17,
        ERROR_KEEPASS_NO_VALID_UUID_PROVIDED = 18
    };
}

class BrowserMessageBuilder
{
public:
    explicit BrowserMessageBuilder() = default;
    static BrowserMessageBuilder* instance();

    QPair<QString, QString> getKeyPair();

    QJsonObject buildMessage(const QString& nonce) const;
    QJsonObject buildResponse(const QString& action,
                              const QJsonObject& message,
                              const QString& nonce,
                              const QString& publicKey,
                              const QString& secretKey);
    QJsonObject getErrorReply(const QString& action, const int errorCode) const;
    QString getErrorMessage(const int errorCode) const;

    QString encryptMessage(const QJsonObject& message,
                           const QString& nonce,
                           const QString& publicKey,
                           const QString& secretKey);
    QJsonObject
    decryptMessage(const QString& message, const QString& nonce, const QString& publicKey, const QString& secretKey);
    QString encrypt(const QString& plaintext, const QString& nonce, const QString& publicKey, const QString& secretKey);
    QByteArray
    decrypt(const QString& encrypted, const QString& nonce, const QString& publicKey, const QString& secretKey);

    QString getBase64FromKey(const uchar* array, const uint len);
    QByteArray getQByteArray(const uchar* array, const uint len) const;
    QJsonObject getJsonObject(const uchar* pArray, const uint len) const;
    QJsonObject getJsonObject(const QByteArray& ba) const;
    QByteArray base64Decode(const QString& str);
    QString incrementNonce(const QString& nonce);

private:
    Q_DISABLE_COPY(BrowserMessageBuilder);

    friend class TestBrowser;
};

static inline BrowserMessageBuilder* browserMessageBuilder()
{
    return BrowserMessageBuilder::instance();
}

#endif // BROWSERMESSAGEBUILDER_H
