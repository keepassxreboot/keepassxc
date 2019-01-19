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
#include "TestGlobal.h"
#include "core/Endian.h"
#include "core/Global.h"
#include "stub/TestRandom.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestRandomGenerator)

void TestRandomGenerator::initTestCase()
{
    m_backend = new RandomBackendPreset();

    TestRandom::setup(m_backend);
}

void TestRandomGenerator::cleanupTestCase()
{
    TestRandom::teardown();

    m_backend = nullptr;
}

void TestRandomGenerator::testUInt()
{
    QByteArray nextBytes;

    nextBytes = Endian::sizedIntToBytes(42, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100), 42U);

    nextBytes = Endian::sizedIntToBytes(117, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100), 17U);

    nextBytes = Endian::sizedIntToBytes(1001, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(1), 0U);

    nextBytes.clear();
    nextBytes.append(Endian::sizedIntToBytes(QUINT32_MAX, QSysInfo::ByteOrder));
    nextBytes.append(Endian::sizedIntToBytes(QUINT32_MAX - 70000U, QSysInfo::ByteOrder));
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100000U), (QUINT32_MAX - 70000U) % 100000U);

    nextBytes.clear();
    for (int i = 0; i < 10000; i++) {
        nextBytes.append(Endian::sizedIntToBytes((QUINT32_MAX / 2U) + 1U + i, QSysInfo::ByteOrder));
    }
    nextBytes.append(Endian::sizedIntToBytes(QUINT32_MAX / 2U, QSysInfo::ByteOrder));
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt((QUINT32_MAX / 2U) + 1U), QUINT32_MAX / 2U);
}

void TestRandomGenerator::testUIntRange()
{
    QByteArray nextBytes;

    nextBytes = Endian::sizedIntToBytes(42, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUIntRange(100, 200), 142U);
}
