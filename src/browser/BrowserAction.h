/*
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef BROWSERACTION_H
#define BROWSERACTION_H

#include "BrowserService.h"
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QtCore>

class BrowserAction : public QObject
{
    Q_OBJECT

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
        ERROR_KEEPASS_CANNOT_CREATE_NEW_GROUP = 17
    };

public:
    BrowserAction(BrowserService& browserService);
    ~BrowserAction() = default;

    QJsonObject readResponse(const QJsonObject& json);

private:
    QJsonObject handleAction(const QJsonObject& json);
    QJsonObject handleChangePublicKeys(const QJsonObject& json, const QString& action);
    QJsonObject handleGetDatabaseHash(const QJsonObject& json, const QString& action);
    QJsonObject handleAssociate(const QJsonObject& json, const QString& action);
    QJsonObject handleTestAssociate(const QJsonObject& json, const QString& action);
    QJsonObject handleGetLogins(const QJsonObject& json, const QString& action);
    QJsonObject handleGeneratePassword(const QJsonObject& json, const QString& action);
    QJsonObject handleSetLogin(const QJsonObject& json, const QString& action);
    QJsonObject handleLockDatabase(const QJsonObject& json, const QString& action);
    QJsonObject handleGetDatabaseGroups(const QJsonObject& json, const QString& action);
    QJsonObject handleCreateNewGroup(const QJsonObject& json, const QString& action);

    QJsonObject buildMessage(const QString& nonce) const;
    QJsonObject buildResponse(const QString& action, const QJsonObject& message, const QString& nonce);
    QJsonObject getErrorReply(const QString& action, const int errorCode) const;
    QString getErrorMessage(const int errorCode) const;
    QString getReturnValue(const BrowserService::ReturnValue returnValue) const;
    QString getDatabaseHash();
    QString getLegacyDatabaseHash();

    QString encryptMessage(const QJsonObject& message, const QString& nonce);
    QJsonObject decryptMessage(const QString& message, const QString& nonce);
    QString encrypt(const QString& plaintext, const QString& nonce);
    QByteArray decrypt(const QString& encrypted, const QString& nonce);

    QString getBase64FromKey(const uchar* array, const uint len);
    QByteArray getQByteArray(const uchar* array, const uint len) const;
    QJsonObject getJsonObject(const uchar* pArray, const uint len) const;
    QJsonObject getJsonObject(const QByteArray& ba) const;
    QByteArray base64Decode(const QString& str);
    QString incrementNonce(const QString& nonce);

private:
    QMutex m_mutex;
    BrowserService& m_browserService;
    QString m_clientPublicKey;
    QString m_publicKey;
    QString m_secretKey;
    bool m_associated;

    friend class TestBrowser;
};

#endif // BROWSERACTION_H
