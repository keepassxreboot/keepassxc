/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "TestRandomGenerator.h"

#include "core/Global.h"
#include "crypto/Random.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestRandomGenerator)

void TestRandomGenerator::testArray()
{
    auto ba = randomGen()->randomArray(10);
    QCOMPARE(ba.size(), 10);
    QVERIFY(ba != QByteArray(10, '\0'));

    auto ba2 = ba;
    randomGen()->randomize(ba2);
    QVERIFY(ba2 != ba);
}

void TestRandomGenerator::testUInt()
{
    QVERIFY(randomGen()->randomUInt(0) == 0);
    QVERIFY(randomGen()->randomUInt(1) == 0);

    // Run a bunch of trials creating random numbers to ensure we meet the standard
    for (int i = 0; i < 100; ++i) {
        QVERIFY(randomGen()->randomUInt(5) < 5);
        QVERIFY(randomGen()->randomUInt(100) < 100);
        QVERIFY(randomGen()->randomUInt(100000U) < 100000U);
        QVERIFY(randomGen()->randomUInt((QUINT32_MAX / 2U) + 1U) < QUINT32_MAX / 2U + 1U);
    }
}

void TestRandomGenerator::testUIntRange()
{
    // Run a bunch of trials to ensure we stay within the range
    for (int i = 0; i < 100; ++i) {
        auto rand = randomGen()->randomUIntRange(100, 200);
        QVERIFY(rand >= 100);
        QVERIFY(rand < 200);
    }
}
