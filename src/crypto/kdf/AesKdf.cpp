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

#include <QtConcurrent>

#include "format/KeePass2.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "AesKdf.h"

const QList<Kdf::Field> AesKdf::FIELDS = AesKdf::initFields();

QList<Kdf::Field> AesKdf::initFields()
{
    return QList<Kdf::Field> {
            Kdf::Field(static_cast<quint32>(Fields::ROUNDS), "Transform rounds", 1, UINT64_MAX, true),
    };
}

bool AesKdf::transform(const QByteArray& raw, QByteArray& result) const
{
    QByteArray resultLeft;
    QByteArray resultRight;

    QFuture<bool> future = QtConcurrent::run(transformKeyRaw, raw.left(16), m_seed, m_rounds, &resultLeft);

    bool rightResult = transformKeyRaw(raw.right(16), m_seed, m_rounds, &resultRight);
    bool leftResult = future.result();

    if (!rightResult || !leftResult) {
        return false;
    }

    QByteArray transformed;
    transformed.append(resultLeft);
    transformed.append(resultRight);

    result = CryptoHash::hash(transformed, CryptoHash::Sha256);
    return true;
}

bool AesKdf::transformKeyRaw(const QByteArray& key, const QByteArray& seed, quint64 rounds, QByteArray* result)
{
    QByteArray iv(16, 0);
    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt);
    if (!cipher.init(seed, iv)) {
        qWarning("AesKdf::transformKeyRaw: error in SymmetricCipher::init: %s", cipher.errorString().toUtf8().data());
        return false;
    }

    *result = key;

    if (!cipher.processInPlace(*result, rounds)) {
        qWarning("AesKdf::transformKeyRaw: error in SymmetricCipher::processInPlace: %s", cipher.errorString().toUtf8().data());
        return false;
    }

    return true;
}

AesKdf::AesKdf()
    : m_rounds(100000ull)
    , m_seed(QByteArray(32, 0))
{
}

quint64 AesKdf::rounds() const
{
    return m_rounds;
}

QByteArray AesKdf::seed() const
{
    return m_seed;
}

bool AesKdf::setRounds(quint64 rounds)
{
    m_rounds = rounds;
    return true;
}

bool AesKdf::setSeed(const QByteArray& seed)
{
    if (seed.size() != 32) {
        return false;
    }

    m_seed = seed;
    return true;
}

void AesKdf::randomizeTransformSalt()
{
    setSeed(randomGen()->randomArray(32));
}

Kdf::Type AesKdf::type() const
{
    return Kdf::Type::AES;
}

Kdf* AesKdf::clone() const
{
    return static_cast<Kdf*>(new AesKdf(*this));
}

const QList<Kdf::Field> AesKdf::fields() const
{
    return FIELDS;
}

quint64 AesKdf::field(quint32 id) const
{
    switch (static_cast<Fields>(id)) {
        case Fields::ROUNDS:
            return m_rounds;
        default:
            return 0;
    }
}

bool AesKdf::setField(quint32 id, quint64 val)
{
    switch (static_cast<Fields>(id)) {
        case Fields::ROUNDS:
            return setRounds(val);
        default:
            return false;
    }
}

int AesKdf::benchmarkImpl(int msec) const
{
    QByteArray key = QByteArray(16, '\x7E');
    QByteArray seed = QByteArray(32, '\x4B');
    QByteArray iv(16, 0);

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt);
    cipher.init(seed, iv);

    int rounds = 0;
    QElapsedTimer t;
    t.start();
    do {
        if (!cipher.processInPlace(key, 10000)) {
            rounds = -1;
            break;
        }
        rounds += 10000;
    } while (!t.hasExpired(msec));

    return rounds;
}