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
#include <QSharedPointer>
#include "crypto/kdf/AesKdf.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "crypto/CryptoHash.h"

const Uuid KeePass2::CIPHER_AES = Uuid(QByteArray::fromHex("31c1f2e6bf714350be5805216afc5aff"));
const Uuid KeePass2::CIPHER_TWOFISH = Uuid(QByteArray::fromHex("ad68f29f576f4bb9a36ad47af965346c"));
const Uuid KeePass2::CIPHER_CHACHA20 = Uuid(QByteArray::fromHex("D6038A2B8B6F4CB5A524339A31DBB59A"));

const Uuid KeePass2::KDF_AES_KDBX3 = Uuid(QByteArray::fromHex("C9D9F39A628A4460BF740D08C18A4FEA"));
const Uuid KeePass2::KDF_AES_KDBX4 = Uuid(QByteArray::fromHex("7C02BB8279A74AC0927D114A00648238"));
const Uuid KeePass2::KDF_ARGON2 = Uuid(QByteArray::fromHex("EF636DDF8C29444B91F7A9A403E30A0C"));

const QByteArray KeePass2::INNER_STREAM_SALSA20_IV("\xE8\x30\x09\x4B\x97\x20\x5D\x2A");

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

const QList<QPair<Uuid, QString>> KeePass2::CIPHERS{
    qMakePair(KeePass2::CIPHER_AES, QString(QT_TRANSLATE_NOOP("KeePass2", "AES: 256-bit"))),
    qMakePair(KeePass2::CIPHER_TWOFISH, QString(QT_TRANSLATE_NOOP("KeePass2", "Twofish: 256-bit"))),
    qMakePair(KeePass2::CIPHER_CHACHA20, QString(QT_TRANSLATE_NOOP("KeePass2", "ChaCha20: 256-bit")))
};

const QList<QPair<Uuid, QString>> KeePass2::KDFS{
    qMakePair(KeePass2::KDF_ARGON2, QString(QT_TRANSLATE_NOOP("KeePass2", "Argon2 (KDBX 4 – recommended)"))),
    qMakePair(KeePass2::KDF_AES_KDBX4, QString(QT_TRANSLATE_NOOP("KeePass2", "AES-KDF (KDBX 4)"))),
    qMakePair(KeePass2::KDF_AES_KDBX3, QString(QT_TRANSLATE_NOOP("KeePass2", "AES-KDF (KDBX 3.1)")))
};

QByteArray KeePass2::hmacKey(QByteArray masterSeed, QByteArray transformedMasterKey) {
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
    if (uuidBytes.size() != Uuid::Length) {
        return {};
    }

    Uuid kdfUuid(uuidBytes);
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

QVariantMap KeePass2::kdfToParameters(QSharedPointer<Kdf> kdf)
{
    return kdf->writeParameters();
}

QSharedPointer<Kdf> KeePass2::uuidToKdf(const Uuid& uuid)
{
    if (uuid == KDF_AES_KDBX3) {
        return QSharedPointer<AesKdf>::create(true);
    }
    if (uuid == KDF_AES_KDBX4) {
        return QSharedPointer<AesKdf>::create();
    }
    if (uuid == KDF_ARGON2) {
        return QSharedPointer<Argon2Kdf>::create();
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
