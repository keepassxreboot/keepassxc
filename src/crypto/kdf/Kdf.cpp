/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "Kdf.h"

#include "crypto/Random.h"

Kdf::Kdf(const QUuid& uuid)
    : m_rounds(KDF_DEFAULT_ROUNDS)
    , m_seed(QByteArray(KDF_MAX_SEED_SIZE, 0))
    , m_uuid(uuid)
{
}

const QUuid& Kdf::uuid() const
{
    return m_uuid;
}

int Kdf::rounds() const
{
    return m_rounds;
}

QByteArray Kdf::seed() const
{
    return m_seed;
}

bool Kdf::setRounds(int rounds)
{
    if (rounds >= 1 && rounds < INT_MAX) {
        m_rounds = rounds;
        return true;
    }
    m_rounds = 1;
    return false;
}

bool Kdf::setSeed(const QByteArray& seed)
{
    if (seed.size() < KDF_MIN_SEED_SIZE || seed.size() > KDF_MAX_SEED_SIZE) {
        return false;
    }

    m_seed = seed;
    return true;
}

void Kdf::randomizeSeed()
{
    setSeed(randomGen()->randomArray(m_seed.size()));
}
