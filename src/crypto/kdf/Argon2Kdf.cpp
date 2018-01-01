/*
 *  Copyright (C) 2017 KeePassXC Team
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

#include "Argon2Kdf.h"

#include <QtConcurrent>
#include <argon2.h>

#include "format/KeePass2.h"
#include "crypto/CryptoHash.h"

/**
 * KeePass' Argon2 implementation supports all parameters that are defined in the official specification,
 * but only the number of iterations, the memory size and the degree of parallelism can be configured by
 * the user in the database settings dialog. For the other parameters, KeePass chooses reasonable defaults:
 * a 256-bit salt is generated each time the database is saved, the tag length is 256 bits, no secret key
 * or associated data. KeePass uses the latest version of Argon2, v1.3.
 */
Argon2Kdf::Argon2Kdf()
        : Kdf::Kdf(KeePass2::KDF_ARGON2)
        , m_memory(1<<16)
        , m_parallelism(2)
{
    m_rounds = 1;
}

quint32 Argon2Kdf::memory() const
{
    // Convert to Megabytes
    return m_memory / (1<<10);
}

bool Argon2Kdf::setMemory(quint32 memoryMegabytes)
{
    // TODO: add bounds check
    // Convert to Kibibytes
    m_memory = (1<<10) * memoryMegabytes;
    return true;
}

quint32 Argon2Kdf::parallelism() const
{
    return m_parallelism;
}

bool Argon2Kdf::setParallelism(quint32 threads)
{
    // TODO: add bounds check
    m_parallelism = threads;
    return true;
}

bool Argon2Kdf::transform(const QByteArray& raw, QByteArray& result) const
{
    result.clear();
    result.resize(32);

    if (!transformKeyRaw(raw, seed(), rounds(), memory(), parallelism(), result)) {
        return false;
    }

    result = CryptoHash::hash(result, CryptoHash::Sha256);
    return true;
}

bool Argon2Kdf::transformKeyRaw(const QByteArray& key, const QByteArray& seed, int rounds,
                                quint32 memory, quint32 parallelism, QByteArray& result)
{
    // Time Cost, Mem Cost, Threads/Lanes, Password, length, Salt, length, out, length
    int rc = argon2d_hash_raw(rounds, memory, parallelism, key.data(), key.size(),
                              seed.data(), seed.size(), result.data(), result.size());
    if (rc != ARGON2_OK) {
        qWarning("Argon2 error: %s", argon2_error_message(rc));
        return false;
    }

    return true;
}

QSharedPointer<Kdf> Argon2Kdf::clone() const
{
    return QSharedPointer<Argon2Kdf>::create(*this);
}

int Argon2Kdf::benchmarkImpl(int msec) const
{
    QByteArray key = QByteArray(16, '\x7E');
    QByteArray seed = QByteArray(32, '\x4B');

    QElapsedTimer timer;
    timer.start();

    int rounds = 4;

    int rc = argon2d_hash_raw(rounds, m_memory, m_parallelism, key.data(), key.size(), seed.data(), seed.size(), key.data(), key.size());
    if (rc != ARGON2_OK) {
        qWarning("Argon2 error: %s", argon2_error_message(rc));
        return -1;
    }

    return static_cast<int>(rounds * (static_cast<float>(msec) / timer.elapsed()));
}