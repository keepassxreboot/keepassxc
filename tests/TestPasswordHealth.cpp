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

#include "TestPasswordHealth.h"

#include "core/PasswordHealth.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestPasswordHealth)

void TestPasswordHealth::initTestCase()
{
}

void TestPasswordHealth::testNoDb()
{
    const auto empty = PasswordHealth("");
    QCOMPARE(empty.score(), 0);
    QCOMPARE(empty.entropy(), 0.0);
    QCOMPARE(empty.quality(), PasswordHealth::Quality::Bad);
    QVERIFY(!empty.scoreReason().isEmpty());
    QVERIFY(!empty.scoreDetails().isEmpty());

    const auto poor = PasswordHealth("secret");
    QCOMPARE(poor.score(), 6);
    QCOMPARE(int(poor.entropy()), 6);
    QCOMPARE(poor.quality(), PasswordHealth::Quality::Poor);
    QVERIFY(!poor.scoreReason().isEmpty());
    QVERIFY(!poor.scoreDetails().isEmpty());

    const auto weak = PasswordHealth("Yohb2ChR4");
    QCOMPARE(weak.score(), 47);
    QCOMPARE(int(weak.entropy()), 47);
    QCOMPARE(weak.quality(), PasswordHealth::Quality::Weak);
    QVERIFY(!weak.scoreReason().isEmpty());
    QVERIFY(!weak.scoreDetails().isEmpty());

    const auto good = PasswordHealth("MIhIN9UKrgtPL2hp");
    QCOMPARE(good.score(), 78);
    QCOMPARE(int(good.entropy()), 78);
    QCOMPARE(good.quality(), PasswordHealth::Quality::Good);
    QVERIFY(good.scoreReason().isEmpty());
    QVERIFY(good.scoreDetails().isEmpty());

    const auto excellent = PasswordHealth("prompter-ream-oversleep-step-extortion-quarrel-reflected-prefix");
    QCOMPARE(excellent.score(), 164);
    QCOMPARE(int(excellent.entropy()), 164);
    QCOMPARE(excellent.quality(), PasswordHealth::Quality::Excellent);
    QVERIFY(excellent.scoreReason().isEmpty());
    QVERIFY(excellent.scoreDetails().isEmpty());
}
