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

#ifndef KEEPASSXC_BROWSERACTION_H
#define KEEPASSXC_BROWSERACTION_H

#include "BrowserMessageBuilder.h"
#include "BrowserService.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class QLocalSocket;

struct BrowserRequest
{
    QString hash;
    QString nonce;
    QString incrementedNonce;
    QString clientID;
    QJsonObject decrypted;

    inline bool isEmpty() const
    {
        return decrypted.isEmpty();
    }

    inline QJsonArray getArray(const QString& param) const
    {
        return decrypted.value(param).toArray();
    }

    inline bool getBool(const QString& param) const
    {
        return decrypted.value(param).toBool();
    }

    inline QJsonObject getObject(const QString& param) const
    {
        return decrypted.value(param).toObject();
    }

    inline QString getString(const QString& param) const
    {
        return decrypted.value(param).toString();
    }
};

class BrowserAction
{
public:
    explicit BrowserAction() = default;
    ~BrowserAction() = default;

    QJsonObject processClientMessage(QLocalSocket* socket, const QJsonObject& json);

private:
    QJsonObject handleAction(const QJsonObject& message, QLocalSocket* socket);
    QJsonObject handleChangePublicKeys(const QJsonObject& message, const QString& action);
    QJsonObject handleGetDatabaseHash(const BrowserRequest& browserRequest);
    QJsonObject handleAssociate(const BrowserRequest& browserRequest);
    QJsonObject handleTestAssociate(const BrowserRequest& browserRequest);
    QJsonObject handleGetLogins(const BrowserRequest& browserRequest);
    QJsonObject handleGeneratePassword(const BrowserRequest& browserRequest, QLocalSocket* socket);
    QJsonObject handleSetLogin(const BrowserRequest& browserRequest);
    QJsonObject handleLockDatabase(const BrowserRequest& browserRequest);
    QJsonObject handleGetDatabaseGroups(const BrowserRequest& browserRequest);
    QJsonObject handleGetDatabaseEntries(const BrowserRequest& browserRequest);
    QJsonObject handleCreateNewGroup(const BrowserRequest& browserRequest);
    QJsonObject handleGetTotp(const BrowserRequest& browserRequest);
    QJsonObject handleDeleteEntry(const BrowserRequest& browserRequest);
    QJsonObject handleGlobalAutoType(const BrowserRequest& browserRequest);
    QJsonObject handlePasskeysGet(const BrowserRequest& browserRequest);
    QJsonObject handlePasskeysRegister(const BrowserRequest& browserRequest);

    QJsonObject buildResponse(const BrowserRequest& browserRequest, const Parameters& params = {});
    QJsonObject buildErrorResponse(const int errorCode, const BrowserRequest& browserRequest) const;
    QJsonObject buildErrorResponse(const int errorCode, const QString& action = {}, const QString& clientID = {}) const;
    QJsonObject decryptMessage(const QString& message, const QString& nonce);
    BrowserRequest decodeRequest(const QJsonObject& json);
    StringPairList getConnectionKeys(const BrowserRequest& browserRequest);

    static const int MaxUrlLength;

    QString m_clientPublicKey;
    QString m_publicKey;
    QString m_secretKey;
    bool m_associated = false;

    friend class TestBrowser;
};

#endif // KEEPASSXC_BROWSERACTION_H
