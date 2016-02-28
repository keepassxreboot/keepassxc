/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Random.h"

#include <gcrypt.h>

#include "core/Global.h"
#include "crypto/Crypto.h"

class RandomBackendGcrypt : public RandomBackend
{
public:
    void randomize(void* data, int len) override;
};

Random* Random::m_instance(nullptr);

void Random::randomize(QByteArray& ba)
{
    m_backend->randomize(ba.data(), ba.size());
}

QByteArray Random::randomArray(int len)
{
    QByteArray ba;
    ba.resize(len);

    randomize(ba);

    return ba;
}

quint32 Random::randomUInt(quint32 limit)
{
    Q_ASSERT(limit != 0);
    Q_ASSERT(limit <= QUINT32_MAX);

    quint32 rand;
    const quint32 ceil = QUINT32_MAX - (QUINT32_MAX % limit) - 1;

    // To avoid modulo bias:
    // Make sure rand is below the largest number where rand%limit==0
    do {
        m_backend->randomize(&rand, 4);
    } while (rand > ceil);

    return (rand % limit);
}

quint32 Random::randomUIntRange(quint32 min, quint32 max)
{
    return min + randomUInt(max - min);
}

Random* Random::instance()
{
    if (!m_instance) {
        m_instance = new Random(new RandomBackendGcrypt());
    }

    return m_instance;
}

void Random::createWithBackend(RandomBackend* backend)
{
    Q_ASSERT(backend);
    Q_ASSERT(!m_instance);

    m_instance = new Random(backend);
}

Random::Random(RandomBackend* backend)
    : m_backend(backend)
{
}


void RandomBackendGcrypt::randomize(void* data, int len)
{
    Q_ASSERT(Crypto::initalized());

    gcry_randomize(data, len, GCRY_STRONG_RANDOM);
}
