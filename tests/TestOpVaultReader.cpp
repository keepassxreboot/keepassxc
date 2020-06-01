/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "TestOpVaultReader.h"

#include "config-keepassx-tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/OpVaultReader.h"
#include "totp/totp.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QTest>
#include <QUuid>

QTEST_GUILESS_MAIN(TestOpVaultReader)

void TestOpVaultReader::initTestCase()
{
    QVERIFY(Crypto::init());

    m_opVaultPath = QStringLiteral("%1/%2").arg(KEEPASSX_TEST_DATA_DIR, QStringLiteral("/keepassxc.opvault"));

    m_categories = QStringList({QStringLiteral("Login"),
                                QStringLiteral("Credit Card"),
                                QStringLiteral("Secure Note"),
                                QStringLiteral("Identity"),
                                QStringLiteral("Password"),
                                QStringLiteral("Tombstone"),
                                QStringLiteral("Software License"),
                                QStringLiteral("Bank Account"),
                                QStringLiteral("Database"),
                                QStringLiteral("Driver License"),
                                QStringLiteral("Outdoor License"),
                                QStringLiteral("Membership"),
                                QStringLiteral("Passport"),
                                QStringLiteral("Rewards"),
                                QStringLiteral("SSN"),
                                QStringLiteral("Router"),
                                QStringLiteral("Server"),
                                QStringLiteral("Email")});
}

void TestOpVaultReader::testReadIntoDatabase()
{
    QDir opVaultDir(m_opVaultPath);

    OpVaultReader reader;
    QScopedPointer<Database> db(reader.readDatabase(opVaultDir, "a"));
    QVERIFY(db);
    QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));

    // Confirm specific entry details are valid
    auto entry = db->rootGroup()->findEntryByPath("/Login/KeePassXC");
    QVERIFY(entry);
    QCOMPARE(entry->title(), QStringLiteral("KeePassXC"));
    QCOMPARE(entry->username(), QStringLiteral("keepassxc"));
    QCOMPARE(entry->password(), QStringLiteral("opvault"));
    QCOMPARE(entry->url(), QStringLiteral("https://www.keepassxc.org"));
    QCOMPARE(entry->notes(), QStringLiteral("KeePassXC Account"));
    // Check extra URL's
    QCOMPARE(entry->attribute("KP2A_URL_1"), QStringLiteral("https://snapshot.keepassxc.org"));
    // Check TOTP
    QVERIFY(entry->hasTotp());
    // Check attachments
    auto attachments = entry->attachments();
    QCOMPARE(attachments->keys().count(), 1);
    QCOMPARE(*attachments->values().begin(), QByteArray("attachment"));

    // Confirm expired entries
    entry = db->rootGroup()->findEntryByPath("/Login/Expired Login");
    QVERIFY(entry->isExpired());

    // Confirm advanced attributes
    entry = db->rootGroup()->findEntryByPath("/Credit Card/My Credit Card");
    QVERIFY(entry);
    auto attr = entry->attributes();
    QCOMPARE(attr->value("cardholder"), QStringLiteral("Team KeePassXC"));
    QVERIFY(!attr->value("validFrom").isEmpty());
    QCOMPARE(attr->value("details_pin"), QStringLiteral("1234"));
    QVERIFY(attr->isProtected("details_pin"));

    // Confirm address fields
    entry = db->rootGroup()->findEntryByPath("/Identity/Team KeePassXC");
    QVERIFY(entry);
    attr = entry->attributes();
    QCOMPARE(attr->value("address_street"), QStringLiteral("123 Password Lane"));

    // Confirm complex passwords
    entry = db->rootGroup()->findEntryByPath("/Password/Complex Password");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QStringLiteral("HfgcHjEL}iO}^3N!?*cv~O:9GJZQ0>oC"));
    QVERIFY(entry->hasTotp());
    auto totpSettings = entry->totpSettings();
    QCOMPARE(totpSettings->digits, static_cast<unsigned int>(8));
    QCOMPARE(totpSettings->step, static_cast<unsigned int>(45));

    // Confirm trashed entries are sent to the recycle bin
    auto recycleBin = db->metadata()->recycleBin();
    QVERIFY(recycleBin);
    QVERIFY(!recycleBin->isEmpty());
    QVERIFY(recycleBin->findEntryByPath("Trashed Password"));

    // Confirm created groups align with category names
    for (const auto group : db->rootGroup()->children()) {
        if (group == recycleBin) {
            continue;
        }
        QVERIFY2(m_categories.contains(group->name()),
                 qPrintable(QStringLiteral("Invalid group name: %1").arg(group->name())));
        // Confirm each group is not empty
        QVERIFY2(!group->isEmpty(), qPrintable(QStringLiteral("Group %1 is empty").arg(group->name())));
    }
}
