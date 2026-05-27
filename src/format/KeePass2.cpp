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

#include "KeePass2.h"
#include "crypto/CryptoHash.h"
#include "crypto/kdf/AesKdf.h"
#include "crypto/kdf/Argon2Kdf.h"

#define UUID_LENGTH 16

const QUuid KeePass2::CIPHER_AES128 = QUuid("61ab05a1-9464-41c3-8d74-3a563df8dd35");
const QUuid KeePass2::CIPHER_AES256 = QUuid("31c1f2e6-bf71-4350-be58-05216afc5aff");
const QUuid KeePass2::CIPHER_TWOFISH = QUuid("ad68f29f-576f-4bb9-a36a-d47af965346c");
const QUuid KeePass2::CIPHER_CHACHA20 = QUuid("d6038a2b-8b6f-4cb5-a524-339a31dbb59a");

const QUuid KeePass2::KDF_AES_KDBX3 = QUuid("c9d9f39a-628a-4460-bf74-0d08c18a4fea");
const QUuid KeePass2::KDF_AES_KDBX4 = QUuid("7c02bb82-79a7-4ac0-927d-114a00648238");
const QUuid KeePass2::KDF_ARGON2D = QUuid("ef636ddf-8c29-444b-91f7-a9a403e30a0c");
const QUuid KeePass2::KDF_ARGON2ID = QUuid("9e298b19-56db-4773-b23d-fc3ec6f0a1e6");

const QByteArray KeePass2::INNER_STREAM_SALSA20_IV("\xe8\x30\x09\x4b\x97\x20\x5d\x2a");

const QString KeePass2::KDFPARAM_UUID("$UUID");
// AES parameters
const QString KeePass2::KDFPARAM_AES_ROUNDS("R");
const QString KeePass2::KDFPARAM_AES_SEED("S");
// Argon2 parameters
const QString KeePass2::KDFPARAM_ARGON2_SALT("S");
const QString KeePass2::KDFPARAM_ARGON2_PARALLELISM("P");
const QString KeePass2::KDFPARAM_ARGON2_MEMORY("M");
const QString KeePass2::KDFPARAM_ARGON2_ITERATIONS("I");
const QString KeePass2::KDFPARAM_ARGON2_VERSION("V");
const QString KeePass2::KDFPARAM_ARGON2_SECRET("K");
const QString KeePass2::KDFPARAM_ARGON2_ASSOCDATA("A");

const QList<QUuid> KeePass2::CIPHERS{KeePass2::CIPHER_AES256, KeePass2::CIPHER_TWOFISH, KeePass2::CIPHER_CHACHA20};

const QList<QUuid> KeePass2::KDBX4_KDFS{KeePass2::KDF_ARGON2D, KeePass2::KDF_ARGON2ID, KeePass2::KDF_AES_KDBX4};
const QList<QUuid> KeePass2::KDBX3_KDFS{KeePass2::KDF_AES_KDBX3};

QByteArray KeePass2::hmacKey(const QByteArray& masterSeed, const QByteArray& transformedMasterKey)
{
    CryptoHash hmacKeyHash(CryptoHash::Sha512);
    hmacKeyHash.addData(masterSeed);
    hmacKeyHash.addData(transformedMasterKey);
    hmacKeyHash.addData(QByteArray(1, '\x01'));
    return hmacKeyHash.result();
}

/**
 * Create KDF object from KDBX4+ KDF parameters.
 *
 * @param p variant map containing parameters
 * @return initialized KDF
 */
QSharedPointer<Kdf> KeePass2::kdfFromParameters(const QVariantMap& p)
{
    QByteArray uuidBytes = p.value(KDFPARAM_UUID).toByteArray();
    if (uuidBytes.size() != UUID_LENGTH) {
        return {};
    }

    QUuid kdfUuid = QUuid::fromRfc4122(uuidBytes);
    if (kdfUuid == KDF_AES_KDBX3) {
        // upgrade to non-legacy AES-KDF, since KDBX3 doesn't have any KDF parameters
        kdfUuid = KDF_AES_KDBX4;
    }
    QSharedPointer<Kdf> kdf = uuidToKdf(kdfUuid);
    if (kdf.isNull()) {
        return {};
    }

    if (!kdf->processParameters(p)) {
        return {};
    }

    return kdf;
}

QVariantMap KeePass2::kdfToParameters(const QSharedPointer<Kdf>& kdf)
{
    return kdf->writeParameters();
}

QSharedPointer<Kdf> KeePass2::uuidToKdf(const QUuid& uuid)
{
    if (uuid == KDF_AES_KDBX3) {
        return QSharedPointer<AesKdf>::create(true);
    }
    if (uuid == KDF_AES_KDBX4) {
        return QSharedPointer<AesKdf>::create();
    }
    if (uuid == KDF_ARGON2D) {
        return QSharedPointer<Argon2Kdf>::create(Argon2Kdf::Type::Argon2d);
    }
    if (uuid == KDF_ARGON2ID) {
        return QSharedPointer<Argon2Kdf>::create(Argon2Kdf::Type::Argon2id);
    }

    return {};
}

KeePass2::ProtectedStreamAlgo KeePass2::idToProtectedStreamAlgo(quint32 id)
{
    switch (id) {
    case static_cast<quint32>(KeePass2::ProtectedStreamAlgo::ArcFourVariant):
        return KeePass2::ProtectedStreamAlgo::ArcFourVariant;
    case static_cast<quint32>(KeePass2::ProtectedStreamAlgo::Salsa20):
        return KeePass2::ProtectedStreamAlgo::Salsa20;
    case static_cast<quint32>(KeePass2::ProtectedStreamAlgo::ChaCha20):
        return KeePass2::ProtectedStreamAlgo::ChaCha20;
    default:
        return KeePass2::ProtectedStreamAlgo::InvalidProtectedStreamAlgo;
    }
}

QString KeePass2::cipherToString(QUuid cipherUuid)
{
    if (cipherUuid == KeePass2::CIPHER_AES256) {
        return QObject::tr("AES 256-bit");
    } else if (cipherUuid == KeePass2::CIPHER_TWOFISH) {
        return QObject::tr("Twofish 256-bit");
    } else if (cipherUuid == KeePass2::CIPHER_CHACHA20) {
        return QObject::tr("ChaCha20 256-bit");
    }
    return QObject::tr("Invalid Cipher");
}

QString KeePass2::kdfToString(QUuid kdfUuid)
{
    if (kdfUuid == KeePass2::KDF_ARGON2D) {
        return QObject::tr("Argon2d (KDBX 4 – recommended)");
    } else if (kdfUuid == KeePass2::KDF_ARGON2ID) {
        return QObject::tr("Argon2id (KDBX 4)");
    } else if (kdfUuid == KeePass2::KDF_AES_KDBX4) {
        return QObject::tr("AES-KDF (KDBX 4)");
    } else if (kdfUuid == KDF_AES_KDBX3) {
        return QObject::tr("AES-KDF (KDBX 3)");
    }
    return QObject::tr("Invalid KDF");
}
