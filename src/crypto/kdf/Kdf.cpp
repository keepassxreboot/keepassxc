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
#include "Kdf_p.h"

#include <QtConcurrent>

#include "crypto/Random.h"

Kdf::Kdf(Uuid uuid)
    : m_rounds(KDF_DEFAULT_ROUNDS)
    , m_seed(QByteArray(KDF_DEFAULT_SEED_SIZE, 0))
    , m_uuid(uuid)
{
}

Uuid Kdf::uuid() const
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
    if (seed.size() != m_seed.size()) {
        return false;
    }

    m_seed = seed;
    return true;
}

void Kdf::randomizeSeed()
{
    setSeed(randomGen()->randomArray(m_seed.size()));
}

int Kdf::benchmark(int msec) const
{
    BenchmarkThread thread1(msec, this);
    BenchmarkThread thread2(msec, this);

    thread1.start();
    thread2.start();

    thread1.wait();
    thread2.wait();

    return qMax(1, qMin(thread1.rounds(), thread2.rounds()));
}

Kdf::BenchmarkThread::BenchmarkThread(int msec, const Kdf* kdf)
    : m_msec(msec)
    , m_kdf(kdf)
{
}

int Kdf::BenchmarkThread::rounds()
{
    return m_rounds;
}

void Kdf::BenchmarkThread::run()
{
    m_rounds = m_kdf->benchmarkImpl(m_msec);
}
