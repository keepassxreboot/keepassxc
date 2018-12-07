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
#include "TestGlobal.h"
#include "updatecheck/UpdateCheck.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestUpdateCheck)

void TestUpdateCheck::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestUpdateCheck::testCompareVersion()
{
                                       // Remote Version , Installed Version
    QCOMPARE(UpdateCheck::compareVersions(QString("2.4.0"), QString("2.3.4")), true);
    QCOMPARE(UpdateCheck::compareVersions(QString("2.3.0"), QString("2.4.0")), false);
    QCOMPARE(UpdateCheck::compareVersions(QString("2.3.0"), QString("2.3.0")), false);
    QCOMPARE(UpdateCheck::compareVersions(QString("2.3.0"), QString("2.3.0-beta1")), true);
    QCOMPARE(UpdateCheck::compareVersions(QString("2.3.0-beta2"), QString("2.3.0-beta1")), true);
    QCOMPARE(UpdateCheck::compareVersions(QString("2.3.4"), QString("2.4.0-snapshot")), false);
    QCOMPARE(UpdateCheck::compareVersions(QString("invalid"), QString("2.4.0")), false);
    QCOMPARE(UpdateCheck::compareVersions(QString(""), QString("2.4.0")), false);
}
