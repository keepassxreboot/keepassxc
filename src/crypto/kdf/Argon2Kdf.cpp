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

#include "Argon2Kdf.h"

#include <QtConcurrent>

#include "crypto/argon2/argon2.h"
#include "format/KeePass2.h"

/**
 * KeePass' Argon2 implementation supports all parameters that are defined in the official specification,
 * but only the number of iterations, the memory size and the degree of parallelism can be configured by
 * the user in the database settings dialog. For the other parameters, KeePass chooses reasonable defaults:
 * a 256-bit salt is generated each time the database is saved, the tag length is 256 bits, no secret key
 * or associated data. KeePass uses the latest version of Argon2, v1.3.
 */
Argon2Kdf::Argon2Kdf()
        : Kdf::Kdf(KeePass2::KDF_ARGON2)
        , m_version(0x13)
        , m_memory(1 << 16)
        , m_parallelism(static_cast<quint32>(QThread::idealThreadCount()))
{
    m_rounds = 1;
}

quint32 Argon2Kdf::version() const
{
    return m_version;
}

bool Argon2Kdf::setVersion(quint32 version)
{
    // MIN=0x10; MAX=0x13)
    if (version >= 0x10 && version <= 0x13) {
        m_version = version;
        return true;
    }
    m_version = 0x13;
    return false;
}

quint64 Argon2Kdf::memory() const
{
    return m_memory;
}

bool Argon2Kdf::setMemory(quint64 kibibytes)
{
    // MIN=8KB; MAX=2,147,483,648KB
    if (kibibytes >= 8 && kibibytes < (1ULL << 32)) {
        m_memory = kibibytes;
        return true;
    }
    m_memory = 16;
    return false;
}

quint32 Argon2Kdf::parallelism() const
{
    return m_parallelism;
}

bool Argon2Kdf::setParallelism(quint32 threads)
{
    // MIN=1; MAX=16,777,215
    if (threads >= 1 && threads < (1 << 24)) {
        m_parallelism = threads;
        return true;
    }
    m_parallelism = 1;
    return false;
}

bool Argon2Kdf::processParameters(const QVariantMap &p)
{
    QByteArray salt = p.value(KeePass2::KDFPARAM_ARGON2_SALT).toByteArray();
    if (!setSeed(salt)) {
        return false;
    }

    bool ok;
    quint32 version = p.value(KeePass2::KDFPARAM_ARGON2_VERSION).toUInt(&ok);
    if (!ok || !setVersion(version)) {
        return false;
    }

    quint32 lanes = p.value(KeePass2::KDFPARAM_ARGON2_PARALLELISM).toUInt(&ok);
    if (!ok || !setParallelism(lanes)) {
        return false;
    }

    quint64 memory = p.value(KeePass2::KDFPARAM_ARGON2_MEMORY).toULongLong(&ok) / 1024ULL;
    if (!ok || !setMemory(memory)) {
        return false;
    }

    quint64 iterations = p.value(KeePass2::KDFPARAM_ARGON2_ITERATIONS).toULongLong(&ok);
    if (!ok || !setRounds(iterations)) {
        return false;
    }

    /* KeePass2 does not currently implement these parameters
     *
    QByteArray secret = p.value(KeePass2::KDFPARAM_ARGON2_SECRET).toByteArray();
    if (!argon2Kdf->setSecret(secret)) {
        return nullptr;
    }

    QByteArray ad = p.value(KeePass2::KDFPARAM_ARGON2_ASSOCDATA).toByteArray();
    if (!argon2Kdf->setAssocData(ad)) {
        return nullptr;
    }
    */

    return true;
}

QVariantMap Argon2Kdf::writeParameters()
{
    QVariantMap p;
    p.insert(KeePass2::KDFPARAM_UUID, KeePass2::KDF_ARGON2.toByteArray());
    p.insert(KeePass2::KDFPARAM_ARGON2_VERSION, version());
    p.insert(KeePass2::KDFPARAM_ARGON2_PARALLELISM, parallelism());
    p.insert(KeePass2::KDFPARAM_ARGON2_MEMORY, memory() * 1024);
    p.insert(KeePass2::KDFPARAM_ARGON2_ITERATIONS, static_cast<quint64>(rounds()));
    p.insert(KeePass2::KDFPARAM_ARGON2_SALT, seed());

    /* KeePass2 does not currently implement these
     *
    if (!assocData().isEmpty()) {
        p.insert(KeePass2::KDFPARAM_ARGON2_ASSOCDATA, argon2Kdf.assocData());
    }

    if (!secret().isEmpty()) {
        p.insert(KeePass2::KDFPARAM_ARGON2_SECRET, argon2Kdf.secret());
    }
    */

    return p;
}

bool Argon2Kdf::transform(const QByteArray& raw, QByteArray& result) const
{
    result.clear();
    result.resize(32);
    return transformKeyRaw(raw, seed(), version(), rounds(), memory(), parallelism(), result);
}

bool Argon2Kdf::transformKeyRaw(const QByteArray& key, const QByteArray& seed, quint32 version,
                                quint32 rounds, quint64 memory, quint32 parallelism, QByteArray& result)
{
    // Time Cost, Mem Cost, Threads/Lanes, Password, length, Salt, length, out, length
    int rc = argon2_hash(rounds, memory, parallelism, key.data(), key.size(),
                         seed.data(), seed.size(), result.data(), result.size(),
                         nullptr, 0, Argon2_d, version);
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
    if (transformKeyRaw(key, seed, version(), rounds, memory(), parallelism(), key)) {
        return static_cast<int>(rounds * (static_cast<float>(msec) / timer.elapsed()));
    }

    return 1;
}
