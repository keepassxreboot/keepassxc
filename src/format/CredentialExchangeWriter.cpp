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

#include "CredentialExchangeWriter.h"
#include "CredentialExchangeReader.h"

#include "core/Clock.h"
#include "core/Entry.h"

#include <QJsonArray>

static const auto EXPORTER_RPID = QStringLiteral("keepassxc.org");
static const auto EXPORTER_DISPLAY_NAME = QStringLiteral("KeePassXC");

namespace
{
    QJsonObject writeEntry(const Entry* entry)
    {
        // Only passkey entries are supported at the moment
        if (!entry->hasPasskey()) {
            return {};
        }

        QJsonObject entryObject{
            {"id", QString(entry->uuid().toByteArray().toBase64())},
            {"username", entry->username()},
            {"email", QString()}, // There is no email in KeePassXC
        };

        QJsonArray items;
        const auto passkeyUsername = entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_USERNAME);
        const auto credentialId = entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID);
        const auto privateKey = entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_PRIVATE_KEY_PEM);
        const auto rpId = entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_RELYING_PARTY);
        const auto userHandle = entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_USER_HANDLE);

        QJsonArray credentials;
        credentials << QJsonObject{
            {"type", "passkey"},
            {"credentialId", credentialId},
            {"rpId", rpId},
            {"username", passkeyUsername},
            {"userDisplayName", QString()}, // KeePassXC does not store this
            {"userHandle", userHandle},
            {"key", privateKey},
        };
        // TODO: Write fido2Extensions

        const auto& timeInfo = entry->timeInfo();
        const auto creationAt = timeInfo.creationTime().toUTC().toSecsSinceEpoch();
        const auto modifiedAt = timeInfo.lastModificationTime().toUTC().toSecsSinceEpoch();
        items << QJsonObject{{"id", credentialId},
                             {"creationAt", creationAt},
                             {"modifiedAt", modifiedAt},
                             {"title", entry->title()},
                             {"credentials", credentials}};

        entryObject["items"] = items;
        return entryObject;
    }
} // namespace

QJsonObject CredentialExchangeWriter::writeEntries(const QList<Entry*>& entries)
{
    if (entries.isEmpty()) {
        // No entries provided
        return {};
    }

    QJsonObject version{
        {"major", CXF_MAJOR_VERSION},
        {"minor", CXF_MINOR_VERSION},
    };

    QJsonArray accounts;
    for (const auto& entry : entries) {
        if (const auto entryObject = writeEntry(entry); !entryObject.isEmpty()) {
            accounts << entryObject;
        }
    }

    const auto timestamp = static_cast<qint64>(Clock::currentSecondsSinceEpoch());
    return QJsonObject{{"version", version},
                       {"exporterRpId", EXPORTER_RPID},
                       {"exporterDisplayName", EXPORTER_DISPLAY_NAME},
                       {"timestamp", timestamp},
                       {"accounts", accounts}};
}
