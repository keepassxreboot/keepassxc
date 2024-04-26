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

#include "TestUpdateCheck.h"
#include "crypto/Crypto.h"
#include "updatecheck/UpdateChecker.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestUpdateCheck)

void TestUpdateCheck::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestUpdateCheck::testCompareVersion()
{
    // No upgrade
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0"), QLatin1String("2.3.0")), false);

    // First digit upgrade
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.4.0"), QLatin1String("3.0.0")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("3.0.0"), QLatin1String("2.4.0")), false);

    // Second digit upgrade
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.4"), QLatin1String("2.4.0")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.4.0"), QLatin1String("2.3.4")), false);

    // Third digit upgrade
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0"), QLatin1String("2.3.1")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.1"), QLatin1String("2.3.0")), false);

    // Beta builds
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0"), QLatin1String("2.3.0-beta1")), false);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0"), QLatin1String("2.3.1-beta1")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0-beta1"), QLatin1String("2.3.0")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0-beta"), QLatin1String("2.3.0-beta1")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0-beta1"), QLatin1String("2.3.0-beta")), false);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0-beta1"), QLatin1String("2.3.0-beta2")), true);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.0-beta2"), QLatin1String("2.3.0-beta1")), false);

    // Snapshot and invalid data
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.3.4-snapshot"), QLatin1String("2.4.0")), false);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.4.0"), QLatin1String("invalid")), false);
    QCOMPARE(UpdateChecker::compareVersions(QLatin1String("2.4.0"), QLatin1String()), false);
}
