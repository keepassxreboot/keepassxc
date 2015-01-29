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

#include "TestRandom.h"

#include "tests.h"
#include "core/Endian.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestRandom)

void TestRandom::initTestCase()
{
    m_backend = new RandomBackendTest();

    Random::createWithBackend(m_backend);
}

void TestRandom::testUInt()
{
    QByteArray nextBytes;

    nextBytes = Endian::int32ToBytes(42, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100), 42U);

    nextBytes = Endian::int32ToBytes(117, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100), 17U);

    nextBytes = Endian::int32ToBytes(1001, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(1), 0U);

    nextBytes.clear();
    nextBytes.append(Endian::int32ToBytes(QUINT32_MAX, QSysInfo::ByteOrder));
    nextBytes.append(Endian::int32ToBytes(QUINT32_MAX - 70000U, QSysInfo::ByteOrder));
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt(100000U), (QUINT32_MAX - 70000U) % 100000U);

    nextBytes.clear();
    for (int i = 0; i < 10000; i++) {
        nextBytes.append(Endian::int32ToBytes((QUINT32_MAX / 2U) + 1U + i, QSysInfo::ByteOrder));
    }
    nextBytes.append(Endian::int32ToBytes(QUINT32_MAX / 2U, QSysInfo::ByteOrder));
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUInt((QUINT32_MAX / 2U) + 1U), QUINT32_MAX / 2U);
}

void TestRandom::testUIntRange()
{
    QByteArray nextBytes;

    nextBytes = Endian::int32ToBytes(42, QSysInfo::ByteOrder);
    m_backend->setNextBytes(nextBytes);
    QCOMPARE(randomGen()->randomUIntRange(100, 200), 142U);
}


RandomBackendTest::RandomBackendTest()
    : m_bytesIndex(0)
{
}

void RandomBackendTest::randomize(void* data, int len)
{
    QVERIFY(len <= (m_nextBytes.size() - m_bytesIndex));

    char* charData = reinterpret_cast<char*>(data);

    for (int i = 0; i < len; i++) {
        charData[i] = m_nextBytes[m_bytesIndex + i];
    }

    m_bytesIndex += len;
}

void RandomBackendTest::setNextBytes(const QByteArray& nextBytes)
{
    m_nextBytes = nextBytes;
    m_bytesIndex = 0;
}
