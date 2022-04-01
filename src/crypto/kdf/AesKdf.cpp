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

#include "AesKdf.h"

#include <QtConcurrent>

#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "format/KeePass2.h"

AesKdf::AesKdf()
    : Kdf::Kdf(KeePass2::KDF_AES_KDBX4)
{
}

/**
 * @param legacyKdbx3 initialize as legacy KDBX3 KDF
 */
AesKdf::AesKdf(bool legacyKdbx3)
    : Kdf::Kdf(legacyKdbx3 ? KeePass2::KDF_AES_KDBX3 : KeePass2::KDF_AES_KDBX4)
{
}

bool AesKdf::processParameters(const QVariantMap& p)
{
    bool ok;
    int rounds = p.value(KeePass2::KDFPARAM_AES_ROUNDS).toInt(&ok);
    if (!ok || !setRounds(rounds)) {
        return false;
    }

    QByteArray seed = p.value(KeePass2::KDFPARAM_AES_SEED).toByteArray();
    return setSeed(seed);
}

QVariantMap AesKdf::writeParameters()
{
    QVariantMap p;

    // always write old KDBX3 AES-KDF UUID for compatibility with other applications
    p.insert(KeePass2::KDFPARAM_UUID, KeePass2::KDF_AES_KDBX3.toRfc4122());

    p.insert(KeePass2::KDFPARAM_AES_ROUNDS, static_cast<quint64>(rounds()));
    p.insert(KeePass2::KDFPARAM_AES_SEED, seed());
    return p;
}

bool AesKdf::transform(const QByteArray& raw, QByteArray& result) const
{
    return transformKeyRaw(raw, m_seed, m_rounds, &result);
}

bool AesKdf::transformKeyRaw(const QByteArray& key, const QByteArray& seed, int rounds, QByteArray* result)
{
    if (!result) {
        return false;
    }

    auto out = key;
    SymmetricCipher::aesKdf(seed, rounds, out);
    *result = CryptoHash::hash(out, CryptoHash::Sha256);
    return true;
}

QSharedPointer<Kdf> AesKdf::clone() const
{
    return QSharedPointer<AesKdf>::create(*this);
}

int AesKdf::benchmark(int msec) const
{
    QByteArray key(16, '\x7E');
    QByteArray seed(32, '\x4B');

    int trials = 3;
    int rounds = 1000000;

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < trials; ++i) {
        QByteArray result;
        if (!transformKeyRaw(key, seed, rounds, &result)) {
            return rounds;
        }
    }

    return static_cast<int>(rounds * trials * static_cast<float>(msec) / timer.elapsed());
}

QString AesKdf::toString() const
{
    return QObject::tr("AES (%1 rounds)").arg(QString::number(rounds()));
}
