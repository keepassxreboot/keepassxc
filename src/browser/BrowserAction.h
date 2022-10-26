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

#ifndef BROWSERACTION_H
#define BROWSERACTION_H

#include <QString>

class QJsonObject;
class QLocalSocket;

class BrowserAction
{
public:
    explicit BrowserAction() = default;
    ~BrowserAction() = default;

    QJsonObject processClientMessage(QLocalSocket* socket, const QJsonObject& json);

private:
    QJsonObject handleAction(QLocalSocket* socket, const QJsonObject& json);
    QJsonObject handleChangePublicKeys(const QJsonObject& json, const QString& action);
    QJsonObject handleGetDatabaseHash(const QJsonObject& json, const QString& action);
    QJsonObject handleAssociate(const QJsonObject& json, const QString& action);
    QJsonObject handleTestAssociate(const QJsonObject& json, const QString& action);
    QJsonObject handleGetLogins(const QJsonObject& json, const QString& action);
    QJsonObject handleGeneratePassword(QLocalSocket* socket, const QJsonObject& json, const QString& action);
    QJsonObject handleSetLogin(const QJsonObject& json, const QString& action);
    QJsonObject handleLockDatabase(const QJsonObject& json, const QString& action);
    QJsonObject handleGetDatabaseGroups(const QJsonObject& json, const QString& action);
    QJsonObject handleCreateNewGroup(const QJsonObject& json, const QString& action);
    QJsonObject handleGetTotp(const QJsonObject& json, const QString& action);
    QJsonObject handleDeleteEntry(const QJsonObject& json, const QString& action);
    QJsonObject handleGlobalAutoType(const QJsonObject& json, const QString& action);

private:
    QJsonObject buildMessage(const QString& nonce) const;
    QJsonObject buildResponse(const QString& action, const QJsonObject& message, const QString& nonce);
    QJsonObject getErrorReply(const QString& action, const int errorCode) const;
    QJsonObject decryptMessage(const QString& message, const QString& nonce);

private:
    static const int MaxUrlLength;

    QString m_clientPublicKey;
    QString m_publicKey;
    QString m_secretKey;
    bool m_associated = false;

    friend class TestBrowser;
};

#endif // BROWSERACTION_H
