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

#include "TestUpdateCheck.h"
#include "crypto/Crypto.h"
#include "networking/UpdateChecker.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestUpdateCheck)

void TestUpdateCheck::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestUpdateCheck::testCompareVersion_data()
{
    QTest::addColumn<QString>("localVersion");
    QTest::addColumn<QString>("remoteVersion");
    QTest::addColumn<bool>("newer");

    // Every ordered pair is also checked in reverse and against itself.
    const auto addPair = [](const char* name, const QString& older, const QString& newer) {
        QTest::newRow(name) << older << newer << true;
        QTest::newRow(qPrintable(QString(name) + "-reverse")) << newer << older << false;
        QTest::newRow(qPrintable(QString(name) + "-equal")) << older << older << false;
    };
    addPair("major", "2.8.0", "3.0.0");
    addPair("minor", "2.8.0", "2.9.0");
    addPair("patch", "2.8.0", "2.8.1");
    addPair("two-components", "2.8", "3.0.0");
    addPair("four-components", "2.8.0", "2.8.0.1");
    addPair("four-component-value", "2.8.0.2", "2.8.0.10");
    addPair("shorter-but-newer", "2.8.99.99", "2.9");
    addPair("revision", "2.8.0", "2.8.0-1");
    addPair("revision-increment", "2.8.0-1", "2.8.0-2");
    addPair("revision-numeric", "2.8.0-2", "2.8.0-10");
    addPair("beta-to-stable", "2.8.0-beta1", "2.8.0");
    addPair("alpha-to-beta", "2.8.0-alpha1", "2.8.0-beta1");
    addPair("alpha-to-beta-rank", "2.8.0-alpha10", "2.8.0-beta1");
    addPair("alpha-to-stable", "2.8.0-alpha10", "2.8.0");
    addPair("beta-number", "2.8.0-beta2", "2.8.0-beta10");
    addPair("bare-beta", "2.8.0-beta", "2.8.0-beta1");
    addPair("bare-alpha", "2.8.0-alpha", "2.8.0-alpha1");
    addPair("case-insensitive", "2.8.0-ALPHA10", "2.8.0-BETA1");
    addPair("newer-prerelease", "2.8.0", "2.8.1-beta1");
    addPair("beta-to-revision", "2.8.0-beta10", "2.8.0-1");
    addPair("normalized-prerelease", "2.8-beta1", "2.8.0");

    QTest::newRow("trailing-zero") << QString("2.8") << QString("2.8.0") << false;
    QTest::newRow("trailing-zero-reverse") << QString("2.8.0") << QString("2.8") << false;
    QTest::newRow("zero-revision") << QString("2.8.0") << QString("2.8.0-0") << false;
    QTest::newRow("zero-revision-reverse") << QString("2.8.0-0") << QString("2.8.0") << false;
    QTest::newRow("local-snapshot") << QString("0.0.0-snapshot") << QString("3.0.0") << false;
    QTest::newRow("snapshot-case") << QString("2.8.0-SNAPSHOT") << QString("3.0.0") << false;

    for (const auto& invalid :
         {"", "invalid", "2..8", "2.8.0-", "2.8.0-garbage", "2.8.0-beta1junk", "2.8.0-18446744073709551616"}) {
        QTest::newRow(qPrintable(QString("invalid-remote-%1").arg(invalid)))
            << QString("2.8.0") << QString(invalid) << false;
        QTest::newRow(qPrintable(QString("invalid-local-%1").arg(invalid)))
            << QString(invalid) << QString("2.8.0") << false;
    }
}

void TestUpdateCheck::testCompareVersion()
{
    QFETCH(QString, localVersion);
    QFETCH(QString, remoteVersion);
    QFETCH(bool, newer);
    QCOMPARE(UpdateChecker::compareVersions(localVersion, remoteVersion), newer);
}
