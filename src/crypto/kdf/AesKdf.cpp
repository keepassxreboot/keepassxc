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

#include "format/KeePass2.h"
#include "crypto/CryptoHash.h"

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

bool AesKdf::processParameters(const QVariantMap &p)
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
    p.insert(KeePass2::KDFPARAM_UUID, KeePass2::KDF_AES_KDBX3.toByteArray());

    p.insert(KeePass2::KDFPARAM_AES_ROUNDS, static_cast<quint64>(rounds()));
    p.insert(KeePass2::KDFPARAM_AES_SEED, seed());
    return p;
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

bool AesKdf::transformKeyRaw(const QByteArray& key, const QByteArray& seed, int rounds, QByteArray* result)
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
        qWarning("AesKdf::transformKeyRaw: error in SymmetricCipher::processInPlace: %s",
                 cipher.errorString().toUtf8().data());
        return false;
    }

    return true;
}

QSharedPointer<Kdf> AesKdf::clone() const
{
    return QSharedPointer<AesKdf>::create(*this);
}

int AesKdf::benchmarkImpl(int msec) const
{
    QByteArray key = QByteArray(16, '\x7E');
    QByteArray seed = QByteArray(32, '\x4B');
    QByteArray iv(16, 0);

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb, SymmetricCipher::Encrypt);
    cipher.init(seed, iv);

    quint64 rounds = 1000000;
    QElapsedTimer timer;
    timer.start();

    if (!cipher.processInPlace(key, rounds)) {
        return -1;
    }

    return static_cast<int>(rounds * (static_cast<float>(msec) / timer.elapsed()));
}
