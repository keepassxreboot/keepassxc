/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestRandom.h"
#include "TestGlobal.h"

RandomBackendPreset::RandomBackendPreset()
    : m_bytesIndex(0)
{
}

void RandomBackendPreset::randomize(void* data, int len)
{
    QVERIFY(len <= (m_nextBytes.size() - m_bytesIndex));

    char* charData = reinterpret_cast<char*>(data);

    for (int i = 0; i < len; i++) {
        charData[i] = m_nextBytes[m_bytesIndex + i];
    }

    m_bytesIndex += len;
}

void RandomBackendPreset::setNextBytes(const QByteArray& nextBytes)
{
    m_nextBytes = nextBytes;
    m_bytesIndex = 0;
}

void TestRandom::setup(RandomBackend* backend)
{
    Random::setInstance(backend);
}

void TestRandom::teardown()
{
    Random::resetInstance();
}

void RandomBackendNull::randomize(void* data, int len)
{
    char* charData = reinterpret_cast<char*>(data);

    for (int i = 0; i < len; i++) {
        charData[i] = '\0';
    }
}
