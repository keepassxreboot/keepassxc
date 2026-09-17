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
#include "TestCredentialExchange.h"

#include "config-keepassx-tests.h"
#include "core/Entry.h"
#include "format/CredentialExchangeReader.h"
#include "format/CredentialExchangeWriter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

QTEST_GUILESS_MAIN(TestCredentialExchange)

// clang-format off
const QString CredentialExchangeData = R"(
    {
        "version": {
            "major": 1,
            "minor": 0
        },
        "exporterRpId": "exporter.example.com",
        "exporterDisplayName": "Exporter app",
        "timestamp": 1705228800,
        "accounts": [
            {
                "id": "DZSXp7iBQY-Fg-OofakQtQ",
                "username": "jane_smith",
                "email": "jane.smith@example.com",
                "fullName": "Jane Smith",
                "items": [
                    {
                        "id": "akKA3Y0jQRuK7sKplB0Y9w",
                        "creationAt": 1705142400,
                        "modifiedAt": 1705228800,
                        "title": "WebAuthn.io",
                        "subtitle": "johndoe",
                        "credentials": [
                             {
                                "type": "passkey",
                                "credentialId": "Y3JlZGVudGlhbElkRXhhbXBsZQ",
                                "rpId": "webauthn.io",
                                "username": "johndoe",
                                "userDisplayName": "John Doe",
                                "userHandle": "cnEzaNHWcYK3coWZjvoaV1Hj9gnI12mKe2dL2HZVFlY",
                                "key": "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgARu_0sCt20EpgVxb4Puq3Ga5VVLpuTY75ngvZlyq3X6hRANCAASmdk1xLsK0oOlhxIPp0d1ZuS0sT9nf6BZtSelhqvLBW0fOL33l_bXgsr_STUHjCLn8l6gcRJwe7OQvbQubZ1dY",
                                "fido2Extensions": {
                                    "hmacCredentials": {
                                        "algorithm": "hmac-sha256",
                                        "credWithUV": "j3N5T9qLpWz2rYf4vS6lDn1KpQx8E0fRc2a7Bm5nUsw",
                                        "credWithoutUV": "y2R8tL3eWf5qBz0sK4hHn9rVgX7pD1cQm6uTj2aP8Fs"
                                    },
                                    "credBlob": "eyJ1c2VyTmFtZSI6ICJKb2huIERvZSIsICJ1c2VySWQiOiAiamRvZS0wMDEiLCAiZW1haWwiOiAiamRvZUBleGFtcGxlLmNvbSJ9",
                                    "largeBlob": {
                                        "uncompressedSize": 129,
                                        "data": "HYxBCoUwDESvMgcQT-Hqr71ATIMtlKQkkY-3twrDLIb3Zq8tMEMKUfZ7pBR08lNwdDtAEcaN3vXfsuJnVbGZrNhf82PYrl5ma1JTXCGOQkkLaAxETnmBOb53O51GbYwQdslYHw"
                                    },
                                    "payments": true
                                }
                            }
                        ]
                    }
                ]
            }
        ]
    }
)";

// clang-format on

void TestCredentialExchange::initTestCase()
{
    QLocale::setDefault(QLocale::c());
}

void TestCredentialExchange::testCredentialExchangeReader()
{
    const QJsonDocument doc(QJsonDocument::fromJson(CredentialExchangeData.toUtf8()));
    const auto importData = doc.object();

    CredentialExchangeReader reader;
    const auto entries = reader.readEntries(importData);
    QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));

    QVERIFY(!entries.isEmpty());
    const auto firstEntry = entries.first();
    QCOMPARE(firstEntry->username(), QString("jane_smith"));
    QCOMPARE(firstEntry->title(), QString("WebAuthn.io"));
    QCOMPARE(firstEntry->attributes()->value(EntryAttributes::KPEX_PASSKEY_USERNAME), QString("johndoe"));
    QCOMPARE(firstEntry->attributes()->value(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID),
             QString("Y3JlZGVudGlhbElkRXhhbXBsZQ"));
    QCOMPARE(firstEntry->attributes()->value(EntryAttributes::KPEX_PASSKEY_PRIVATE_KEY_PEM),
             QString("MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgARu_"
                     "0sCt20EpgVxb4Puq3Ga5VVLpuTY75ngvZlyq3X6hRANCAASmdk1xLsK0oOlhxIPp0d1ZuS0sT9nf6BZtSelhqvLBW0fOL33l_"
                     "bXgsr_STUHjCLn8l6gcRJwe7OQvbQubZ1dY"));
    QCOMPARE(firstEntry->attributes()->value(EntryAttributes::KPEX_PASSKEY_RELYING_PARTY), QString("webauthn.io"));
    QCOMPARE(firstEntry->attributes()->value(EntryAttributes::KPEX_PASSKEY_USER_HANDLE),
             QString("cnEzaNHWcYK3coWZjvoaV1Hj9gnI12mKe2dL2HZVFlY"));
    QCOMPARE(firstEntry->timeInfo().creationTime().toString(Qt::ISODate), QString("2024-01-13T10:40:00Z"));
    QCOMPARE(firstEntry->timeInfo().lastModificationTime().toString(Qt::ISODate), QString("2024-01-14T10:40:00Z"));
}

void TestCredentialExchange::testCredentialExchangeWriter()
{
    const auto firstEntry = new Entry();
    firstEntry->setUuid(QUuid::createUuid());
    firstEntry->setUsername("John Doe");
    firstEntry->setTitle("Title for John Doe");
    firstEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USERNAME, "johndoe");
    firstEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID, "Y3JlZGVudGlhbElkRXhhbXBsZQ");
    firstEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_PRIVATE_KEY_PEM, "MIGHAgEAMBMGByqGS...");
    firstEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_RELYING_PARTY, "webauthn.io");
    firstEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USER_HANDLE,
                                  "cnEzaNHWcYK3coWZjvoaV1Hj9gnI12mKe2dL2HZVFlY");

    const auto secondEntry = new Entry();
    secondEntry->setUuid(QUuid::createUuid());
    secondEntry->setUsername("Jane Doe");
    secondEntry->setTitle("Title for Jane Doe");
    secondEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USERNAME, "janedoe");
    secondEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID, "GVuElkRXhhbXBsZdGlhY3JlZb");
    secondEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_PRIVATE_KEY_PEM, "XIGHAgEAMBMGByqGS...");
    secondEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_RELYING_PARTY, "webauthn.io");
    secondEntry->attributes()->set(EntryAttributes::KPEX_PASSKEY_USER_HANDLE,
                                   "K3coWgnI12mZjvoaV1Hj92HZVFlYKecnEzaNHWcY2dL");

    QList<Entry*> entries;
    entries << firstEntry << secondEntry;

    CredentialExchangeWriter writer;
    const auto result = writer.writeEntries(entries);
    QVERIFY(result["accounts"].toArray().size() == 2);
}
