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

#include "BrowserAction.h"
#include "BrowserMessageBuilder.h"
#include "BrowserService.h"
#include "BrowserSettings.h"
#include "BrowserShared.h"
#include "config-keepassx.h"
#include "core/Global.h"
#include "core/Tools.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

const int BrowserAction::MaxUrlLength = 256;

QJsonObject BrowserAction::processClientMessage(QLocalSocket* socket, const QJsonObject& json)
{
    if (json.isEmpty()) {
        return getErrorReply("", ERROR_KEEPASS_EMPTY_MESSAGE_RECEIVED);
    }

    bool triggerUnlock = false;
    const QString trigger = json.value("triggerUnlock").toString();
    if (!trigger.isEmpty() && trigger.compare(TRUE_STR) == 0) {
        triggerUnlock = true;
    }

    const QString action = json.value("action").toString();
    if (action.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    if (action.compare("change-public-keys") != 0 && action.compare("request-autotype") != 0
        && !browserService()->isDatabaseOpened()) {
        if (m_clientPublicKey.isEmpty()) {
            return getErrorReply(action, ERROR_KEEPASS_CLIENT_PUBLIC_KEY_NOT_RECEIVED);
        } else if (!browserService()->openDatabase(triggerUnlock)) {
            return getErrorReply(action, ERROR_KEEPASS_DATABASE_NOT_OPENED);
        }
    }

    return handleAction(socket, json);
}

// Private functions
///////////////////////

QJsonObject BrowserAction::handleAction(QLocalSocket* socket, const QJsonObject& json)
{
    QString action = json.value("action").toString();

    if (action.compare("change-public-keys") == 0) {
        return handleChangePublicKeys(json, action);
    } else if (action.compare("get-databasehash") == 0) {
        return handleGetDatabaseHash(json, action);
    } else if (action.compare("associate") == 0) {
        return handleAssociate(json, action);
    } else if (action.compare("test-associate") == 0) {
        return handleTestAssociate(json, action);
    } else if (action.compare("get-logins") == 0) {
        return handleGetLogins(json, action);
    } else if (action.compare("generate-password") == 0) {
        return handleGeneratePassword(socket, json, action);
    } else if (action.compare("set-login") == 0) {
        return handleSetLogin(json, action);
    } else if (action.compare("lock-database") == 0) {
        return handleLockDatabase(json, action);
    } else if (action.compare("get-database-groups") == 0) {
        return handleGetDatabaseGroups(json, action);
    } else if (action.compare("create-new-group") == 0) {
        return handleCreateNewGroup(json, action);
    } else if (action.compare("get-totp") == 0) {
        return handleGetTotp(json, action);
    } else if (action.compare("delete-entry") == 0) {
        return handleDeleteEntry(json, action);
    } else if (action.compare("request-autotype") == 0) {
        return handleGlobalAutoType(json, action);
    }

    // Action was not recognized
    return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
}

QJsonObject BrowserAction::handleChangePublicKeys(const QJsonObject& json, const QString& action)
{
    const QString nonce = json.value("nonce").toString();
    const QString clientPublicKey = json.value("publicKey").toString();

    if (clientPublicKey.isEmpty() || nonce.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CLIENT_PUBLIC_KEY_NOT_RECEIVED);
    }

    m_associated = false;
    auto keyPair = browserMessageBuilder()->getKeyPair();
    if (keyPair.first.isEmpty() || keyPair.second.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_ENCRYPTION_KEY_UNRECOGNIZED);
    }

    m_clientPublicKey = clientPublicKey;
    m_publicKey = keyPair.first;
    m_secretKey = keyPair.second;

    QJsonObject response = browserMessageBuilder()->buildMessage(browserMessageBuilder()->incrementNonce(nonce));
    response["action"] = action;
    response["publicKey"] = keyPair.first;

    return response;
}

QJsonObject BrowserAction::handleGetDatabaseHash(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);

    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    if (hash.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_DATABASE_HASH_NOT_RECEIVED);
    }

    QString command = decrypted.value("action").toString();
    if (!command.isEmpty() && command.compare("get-databasehash") == 0) {
        const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

        QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
        message["hash"] = hash;

        // Update a legacy database hash if found
        const QJsonArray hashes = decrypted.value("connectedKeys").toArray();
        if (!hashes.isEmpty()) {
            const QString legacyHash = browserService()->getDatabaseHash(true);
            if (hashes.contains(legacyHash)) {
                message["oldHash"] = legacyHash;
            }
        }

        return buildResponse(action, message, newNonce);
    }

    return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
}

QJsonObject BrowserAction::handleAssociate(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);

    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    const QString key = decrypted.value("key").toString();
    if (key.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    if (key.compare(m_clientPublicKey) == 0) {
        // Check for identification key. If it's not found, ensure backwards compatibility and use the current public
        // key
        const QString idKey = decrypted.value("idKey").toString();
        const QString id = browserService()->storeKey((idKey.isEmpty() ? key : idKey));
        if (id.isEmpty()) {
            return getErrorReply(action, ERROR_KEEPASS_ACTION_CANCELLED_OR_DENIED);
        }

        m_associated = true;
        const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

        QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
        message["hash"] = hash;
        message["id"] = id;
        return buildResponse(action, message, newNonce);
    }

    return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
}

QJsonObject BrowserAction::handleTestAssociate(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);

    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    const QString responseKey = decrypted.value("key").toString();
    const QString id = decrypted.value("id").toString();
    if (responseKey.isEmpty() || id.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_DATABASE_NOT_OPENED);
    }

    const QString key = browserService()->getKey(id);
    if (key.isEmpty() || key.compare(responseKey) != 0) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    m_associated = true;
    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["hash"] = hash;
    message["id"] = id;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleGetLogins(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    const QString siteUrl = decrypted.value("url").toString();
    if (siteUrl.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_NO_URL_PROVIDED);
    }

    const QJsonArray keys = decrypted.value("keys").toArray();

    StringPairList keyList;
    for (const QJsonValue val : keys) {
        const QJsonObject keyObject = val.toObject();
        keyList.push_back(qMakePair(keyObject.value("id").toString(), keyObject.value("key").toString()));
    }

    const QString id = decrypted.value("id").toString();
    const QString formUrl = decrypted.value("submitUrl").toString();
    const QString auth = decrypted.value("httpAuth").toString();
    const bool httpAuth = auth.compare(TRUE_STR) == 0;
    const QJsonArray users = browserService()->findMatchingEntries(id, siteUrl, formUrl, "", keyList, httpAuth);

    if (users.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_NO_LOGINS_FOUND);
    }

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["count"] = users.count();
    message["entries"] = users;
    message["hash"] = hash;
    message["id"] = id;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleGeneratePassword(QLocalSocket* socket, const QJsonObject& json, const QString& action)
{
    auto errorMessage = getErrorReply(action, ERROR_KEEPASS_ACTION_CANCELLED_OR_DENIED);
    auto nonce = json.value("nonce").toString();
    auto incrementedNonce = browserMessageBuilder()->incrementNonce(nonce);

    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    auto requestId = decrypted.value("requestID").toString();

    // Do not allow multiple requests from the same client
    if (browserService()->isPasswordGeneratorRequested()) {
        auto errorReply = getErrorReply(action, ERROR_KEEPASS_ACTION_CANCELLED_OR_DENIED);

        // Append requestID to the response if found
        if (!requestId.isEmpty()) {
            errorReply["requestID"] = requestId;
        }

        return errorReply;
    }

    browserService()->showPasswordGenerator(socket, incrementedNonce, m_clientPublicKey, m_secretKey);
    return QJsonObject();
}

QJsonObject BrowserAction::handleSetLogin(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    const QString url = decrypted.value("url").toString();
    if (url.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_NO_URL_PROVIDED);
    }

    const QString id = decrypted.value("id").toString();
    const QString login = decrypted.value("login").toString();
    const QString password = decrypted.value("password").toString();
    const QString submitUrl = decrypted.value("submitUrl").toString();
    const QString uuid = decrypted.value("uuid").toString();
    const QString group = decrypted.value("group").toString();
    const QString groupUuid = decrypted.value("groupUuid").toString();
    const QString downloadFavicon = decrypted.value("downloadFavicon").toString();
    const QString realm;

    bool result = true;
    if (uuid.isEmpty()) {
        auto dlFavicon = !downloadFavicon.isEmpty() && downloadFavicon.compare(TRUE_STR) == 0;
        browserService()->addEntry(id, login, password, url, submitUrl, realm, group, groupUuid, dlFavicon);
    } else {
        if (!Tools::isValidUuid(uuid)) {
            return getErrorReply(action, ERROR_KEEPASS_NO_VALID_UUID_PROVIDED);
        }

        result = browserService()->updateEntry(id, uuid, login, password, url, submitUrl);
    }

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["count"] = QJsonValue::Null;
    message["entries"] = QJsonValue::Null;
    message["error"] = result ? QStringLiteral("success") : QStringLiteral("error");
    message["hash"] = hash;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleLockDatabase(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);

    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    if (hash.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_DATABASE_HASH_NOT_RECEIVED);
    }

    QString command = decrypted.value("action").toString();
    if (!command.isEmpty() && command.compare("lock-database") == 0) {
        browserService()->lockDatabase();

        const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);
        QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);

        return buildResponse(action, message, newNonce);
    }

    return getErrorReply(action, ERROR_KEEPASS_DATABASE_HASH_NOT_RECEIVED);
}

QJsonObject BrowserAction::handleGetDatabaseGroups(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    QString command = decrypted.value("action").toString();
    if (command.isEmpty() || command.compare("get-database-groups") != 0) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    const QJsonObject groups = browserService()->getDatabaseGroups();
    if (groups.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_NO_GROUPS_FOUND);
    }

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["groups"] = groups;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleCreateNewGroup(const QJsonObject& json, const QString& action)
{
    const QString hash = browserService()->getDatabaseHash();
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    QString command = decrypted.value("action").toString();
    if (command.isEmpty() || command.compare("create-new-group") != 0) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    QString group = decrypted.value("groupName").toString();
    const QJsonObject newGroup = browserService()->createNewGroup(group);
    if (newGroup.isEmpty() || newGroup["name"].toString().isEmpty() || newGroup["uuid"].toString().isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_CREATE_NEW_GROUP);
    }

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["name"] = newGroup["name"];
    message["uuid"] = newGroup["uuid"];

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleGetTotp(const QJsonObject& json, const QString& action)
{
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    QString command = decrypted.value("action").toString();
    if (command.isEmpty() || command.compare("get-totp") != 0) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    const QString uuid = decrypted.value("uuid").toString();
    if (!Tools::isValidUuid(uuid)) {
        return getErrorReply(action, ERROR_KEEPASS_NO_VALID_UUID_PROVIDED);
    }

    // Get the current TOTP
    const auto totp = browserService()->getCurrentTotp(uuid);
    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);

    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["totp"] = totp;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleDeleteEntry(const QJsonObject& json, const QString& action)
{
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();

    if (!m_associated) {
        return getErrorReply(action, ERROR_KEEPASS_ASSOCIATION_FAILED);
    }

    const QJsonObject decrypted = decryptMessage(encrypted, nonce);
    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    QString command = decrypted.value("action").toString();
    if (command.isEmpty() || command.compare("delete-entry") != 0) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    const auto uuid = decrypted.value("uuid").toString();
    if (!Tools::isValidUuid(uuid)) {
        return getErrorReply(action, ERROR_KEEPASS_NO_VALID_UUID_PROVIDED);
    }

    const auto result = browserService()->deleteEntry(uuid);

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);
    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    message["success"] = result ? TRUE_STR : FALSE_STR;

    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::handleGlobalAutoType(const QJsonObject& json, const QString& action)
{
    const QString nonce = json.value("nonce").toString();
    const QString encrypted = json.value("message").toString();
    const QJsonObject decrypted = decryptMessage(encrypted, nonce);

    if (decrypted.isEmpty()) {
        return getErrorReply(action, ERROR_KEEPASS_CANNOT_DECRYPT_MESSAGE);
    }

    QString command = decrypted.value("action").toString();
    if (command.isEmpty() || command.compare("request-autotype") != 0) {
        return getErrorReply(action, ERROR_KEEPASS_INCORRECT_ACTION);
    }

    const auto topLevelDomain = decrypted.value("search").toString();
    if (topLevelDomain.length() > BrowserAction::MaxUrlLength) {
        return getErrorReply(action, ERROR_KEEPASS_NO_URL_PROVIDED);
    }

    browserService()->requestGlobalAutoType(topLevelDomain);

    const QString newNonce = browserMessageBuilder()->incrementNonce(nonce);
    QJsonObject message = browserMessageBuilder()->buildMessage(newNonce);
    return buildResponse(action, message, newNonce);
}

QJsonObject BrowserAction::decryptMessage(const QString& message, const QString& nonce)
{
    return browserMessageBuilder()->decryptMessage(message, nonce, m_clientPublicKey, m_secretKey);
}

QJsonObject BrowserAction::getErrorReply(const QString& action, const int errorCode) const
{
    return browserMessageBuilder()->getErrorReply(action, errorCode);
}

QJsonObject BrowserAction::buildResponse(const QString& action, const QJsonObject& message, const QString& nonce)
{
    return browserMessageBuilder()->buildResponse(action, message, nonce, m_clientPublicKey, m_secretKey);
}
