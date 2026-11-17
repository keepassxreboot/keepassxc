/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CredentialExchangeReader.h"

#include "core/Entry.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTimeZone>

static const auto CREDENTIAL_PASSKEY = QStringLiteral("passkey");

namespace
{
    // Parse created and modified timestamps
    void setTimeInfo(const QScopedPointer<Entry>& entry, const QJsonObject& item)
    {
        auto timeInfo = entry->timeInfo();
        const auto creationAt = item["creationAt"].toInteger();
        const auto modifiedAt = item["modifiedAt"].toInteger();
        const auto creationTimestamp = QDateTime::fromSecsSinceEpoch(creationAt).toUTC();
        const auto modifiedTimestamp = QDateTime::fromSecsSinceEpoch(modifiedAt).toUTC();
        timeInfo.setCreationTime(creationTimestamp);
        timeInfo.setLastModificationTime(modifiedTimestamp);
        entry->setTimeInfo(timeInfo);
    }

    // Parse credential of "passkey" type
    void setPasskeyCredential(const QScopedPointer<Entry>& entry, const QJsonObject& credential)
    {
        const auto credentialId = credential["credentialId"].toString();
        const auto rpId = credential["rpId"].toString();
        const auto passkeyUsername = credential["username"].toString();
        const auto userDisplayName = credential["userDisplayName"].toString();
        const auto userHandle = credential["userHandle"].toString();
        // The value MUST be PKCS#8 ASN.1 DER formatted byte string which is then Base64url encoded.
        const auto privateKey = credential["key"].toString();

        // TODO: fido2Extensions.hmacCredentials should be handled after PRF support has been made.
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USERNAME, passkeyUsername);
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID, credentialId, true);
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_PRIVATE_KEY_PEM, privateKey, true);
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_RELYING_PARTY, rpId);
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USER_HANDLE, userHandle, true);
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_FLAG_BE, "1");
        entry->attributes()->set(EntryAttributes::KPEX_PASSKEY_FLAG_BS, "1");
    }

    // Account can has multiple items. Create a new entry for an Account, and parse Items to it.
    // Credentials under Item can contain multple objects. Only passkey credentials are supported for now.
    // TODO: It needs to be decided if each Item needs a separate entry, or is one Account one entry, and each
    // Item is added to it. The Account doesn't have a common title, but title can vary per each Item.
    Entry* readAccount(const QJsonObject& account)
    {
        QScopedPointer<Entry> entry(new Entry());

        // Account info
        const auto username = account["username"].toString();
        const auto email = account["email"].toString();
        // KeePassXC does not have a separate email field. Use email as username if username is not set.
        entry->setUsername(username.isEmpty() ? email : username);

        // Parse Item entities
        const auto items = account["items"].toArray();
        for (const auto& i : items) {
            const auto item = i.toObject();

            entry->setEmitModified(false);
            entry->setUuid(QUuid::createUuid());

            // TODO: How to handle titles if there are multiple items?
            entry->setTitle(item["title"].toString());
            setTimeInfo(entry, item);

            // Parse item credentials
            const auto credentials = item["credentials"].toArray();
            for (const auto& cred : credentials) {
                const auto credential = cred.toObject();
                const auto credentialType = credential["type"].toString();

                // Only support passkeys for now
                if (credentialType == CREDENTIAL_PASSKEY) {
                    setPasskeyCredential(entry, credential);
                }
            }

            entry->setEmitModified(true);
        }

        return entry.take();
    }
} // namespace

bool CredentialExchangeReader::hasError()
{
    return !m_error.isEmpty();
}

QString CredentialExchangeReader::errorString()
{
    return m_error;
}

QList<Entry*> CredentialExchangeReader::readEntries(const QJsonObject& data)
{
    // Verify version
    const auto version = data["version"].toObject();
    if (version.isEmpty()) {
        m_error = QObject::tr("Version information not found.");
        return {};
    }

    // Minor versions should be compatible
    const auto majorVersion = version["major"].toInt();
    if (majorVersion < CXF_MAJOR_VERSION) {
        m_error = QObject::tr("Major version %1 is not supported.").arg(majorVersion);
        return {};
    }

    // Check exporter and timestamp exists
    if (data["exporterRpId"].toString().isEmpty() || data["exporterDisplayName"].toString().isEmpty()
        || data["timestamp"].toInteger() == 0) {
        m_error = QObject::tr("No exporter data or timestamp provided.");
        return {};
    }

    // Parse accounts
    const auto accounts = data["accounts"].toArray();
    if (accounts.isEmpty()) {
        // No accounts to import
        m_error = QObject::tr("No accounts to read.");
        return {};
    }

    QList<Entry*> entries;
    for (const auto& account : accounts) {
        const auto accountEntry = readAccount(account.toObject());
        if (accountEntry) {
            entries << accountEntry;
        }
    }

    if (entries.isEmpty()) {
        m_error = QObject::tr("No accounts were succesfully read.");
    }
    return entries;
}
