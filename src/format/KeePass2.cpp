/*
 *  Copyright (C) 2017 angelsl
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

const Uuid KeePass2::CIPHER_AES = Uuid(QByteArray::fromHex("31c1f2e6bf714350be5805216afc5aff"));
const Uuid KeePass2::CIPHER_TWOFISH = Uuid(QByteArray::fromHex("ad68f29f576f4bb9a36ad47af965346c"));
const Uuid KeePass2::CIPHER_CHACHA20 = Uuid(QByteArray::fromHex("D6038A2B8B6F4CB5A524339A31DBB59A"));

const Uuid KeePass2::KDF_AES = Uuid(QByteArray::fromHex("C9D9F39A628A4460BF740D08C18A4FEA"));
const Uuid KeePass2::KDF_ARGON2 = Uuid(QByteArray::fromHex("EF636DDF8C29444B91F7A9A403E30A0C"));

const QByteArray KeePass2::INNER_STREAM_SALSA20_IV("\xE8\x30\x09\x4B\x97\x20\x5D\x2A");

const QString KeePass2::KDFPARAM_UUID("$UUID");
const QString KeePass2::KDFPARAM_AES_ROUNDS("R");
const QString KeePass2::KDFPARAM_AES_SEED("S");
const QString KeePass2::KDFPARAM_ARGON2_SALT("S");
const QString KeePass2::KDFPARAM_ARGON2_LANES("P");
const QString KeePass2::KDFPARAM_ARGON2_MEMORY("M");
const QString KeePass2::KDFPARAM_ARGON2_TIME("I");
const QString KeePass2::KDFPARAM_ARGON2_VERSION("V");
const QString KeePass2::KDFPARAM_ARGON2_SECRET("K");
const QString KeePass2::KDFPARAM_ARGON2_ASSOCDATA("A");

const QList<KeePass2::UuidNamePair> KeePass2::CIPHERS {
        KeePass2::UuidNamePair(KeePass2::CIPHER_AES, "AES: 256-bit"),
        KeePass2::UuidNamePair(KeePass2::CIPHER_TWOFISH, "Twofish: 256-bit"),
        KeePass2::UuidNamePair(KeePass2::CIPHER_CHACHA20, "ChaCha20: 256-bit")
};
const QList<KeePass2::UuidNamePair> KeePass2::KDFS {
        KeePass2::UuidNamePair(KeePass2::KDF_AES, "AES-KDF"),
        KeePass2::UuidNamePair(KeePass2::KDF_ARGON2, "Argon2"),
};

QByteArray KeePass2::hmacKey(QByteArray masterSeed, QByteArray transformedMasterKey) {
    CryptoHash hmacKeyHash(CryptoHash::Sha512);
    hmacKeyHash.addData(masterSeed);
    hmacKeyHash.addData(transformedMasterKey);
    hmacKeyHash.addData(QByteArray(1, '\x01'));
    return hmacKeyHash.result();
}

Kdf* KeePass2::paramsToKdf(const QVariantMap& p)
{
    QByteArray uuidBytes = p.value(KDFPARAM_UUID).toByteArray();
    if (uuidBytes.size() != Uuid::Length) {
        return nullptr;
    }

    QScopedPointer<Kdf> kdf(uuidToKdf(Uuid(uuidBytes)));
    if (kdf.isNull()) {
        return nullptr;
    }

    switch (kdf->type()) {
    case Kdf::Type::AES: {
        QScopedPointer<AesKdf> aesKdf(static_cast<AesKdf*>(kdf.take()));

        bool ok;
        quint64 rounds = p.value(KDFPARAM_AES_ROUNDS).toULongLong(&ok);
        if (!ok || !aesKdf->setRounds(rounds)) {
            return nullptr;
        }

        QByteArray seed = p.value(KDFPARAM_AES_SEED).toByteArray();
        if (!aesKdf->setSeed(seed)) {
            return nullptr;
        }

        return static_cast<Kdf*>(aesKdf.take());
    }
    case Kdf::Type::ARGON2: {
        QScopedPointer<Argon2Kdf> argon2Kdf(static_cast<Argon2Kdf*>(kdf.take()));

        QByteArray salt = p.value(KDFPARAM_ARGON2_SALT).toByteArray();
        if (!argon2Kdf->setSalt(salt)) {
            return nullptr;
        }

        bool ok;
        quint32 lanes = p.value(KDFPARAM_ARGON2_LANES).toUInt(&ok);
        if (!ok || !argon2Kdf->setLanes(lanes)) {
            return nullptr;
        }

        quint64 memory = p.value(KDFPARAM_ARGON2_MEMORY).toULongLong(&ok);
        if (!ok || !argon2Kdf->setMemoryCost(memory)) {
            return nullptr;
        }

        quint64 time = p.value(KDFPARAM_ARGON2_TIME).toULongLong(&ok);
        if (!ok || !argon2Kdf->setTimeCost(time)) {
            return nullptr;
        }

        quint32 version = p.value(KDFPARAM_ARGON2_VERSION).toUInt(&ok);
        if (!ok || !argon2Kdf->setVersion(version)) {
            return nullptr;
        }

        QByteArray secret = p.value(KDFPARAM_ARGON2_SECRET).toByteArray();
        if (!argon2Kdf->setSecret(secret)) {
            return nullptr;
        }

        QByteArray ad = p.value(KDFPARAM_ARGON2_ASSOCDATA).toByteArray();
        if (!argon2Kdf->setAssocData(ad)) {
            return nullptr;
        }

        return static_cast<Kdf*>(argon2Kdf.take());
    }
    }

    return nullptr;
}

Kdf* KeePass2::uuidToKdf(const Uuid& uuid) {
    if (uuid == KDF_AES) {
        return static_cast<Kdf*>(new AesKdf());
    } else if (uuid == KDF_ARGON2) {
        return static_cast<Kdf*>(new Argon2Kdf());
    }

    return nullptr;
}

bool KeePass2::kdfToParams(const Kdf& kdf, QVariantMap& r)
{
    switch (kdf.type()) {
    case Kdf::Type::AES: {
        const AesKdf& aesKdf = static_cast<const AesKdf&>(kdf);
        r.clear();
        r.insert(KeePass2::KDFPARAM_UUID, KeePass2::KDF_AES.toByteArray());
        r.insert(KeePass2::KDFPARAM_AES_ROUNDS, aesKdf.rounds());
        r.insert(KeePass2::KDFPARAM_AES_SEED, aesKdf.seed());

        return true;
    }
    case Kdf::Type::ARGON2: {
        const Argon2Kdf& argon2Kdf = static_cast<const Argon2Kdf&>(kdf);
        r.clear();
        r.insert(KeePass2::KDFPARAM_UUID, KDF_ARGON2.toByteArray());
        r.insert(KeePass2::KDFPARAM_ARGON2_VERSION, argon2Kdf.version());
        r.insert(KeePass2::KDFPARAM_ARGON2_LANES, argon2Kdf.lanes());
        r.insert(KeePass2::KDFPARAM_ARGON2_MEMORY, argon2Kdf.memoryCost());
        r.insert(KeePass2::KDFPARAM_ARGON2_TIME, argon2Kdf.timeCost());
        r.insert(KeePass2::KDFPARAM_ARGON2_SALT, argon2Kdf.salt());

        if (!argon2Kdf.assocData().isEmpty()) {
            r.insert(KeePass2::KDFPARAM_ARGON2_ASSOCDATA, argon2Kdf.assocData());
        }

        if (!argon2Kdf.secret().isEmpty()) {
            r.insert(KeePass2::KDFPARAM_ARGON2_SECRET, argon2Kdf.secret());
        }

        return true;
    }
    }

    return false;
}

Uuid KeePass2::kdfToUuid(const Kdf& kdf)
{
    switch (kdf.type()) {
        case Kdf::Type::AES:
            return KDF_AES;
        case Kdf::Type::ARGON2:
            return KDF_ARGON2;
        default:
            return Uuid();
    }
}

KeePass2::UuidNamePair::UuidNamePair(const Uuid& uuid, const QString& name)
    : m_uuid(uuid)
    , m_name(name)
{
}

Uuid KeePass2::UuidNamePair::uuid() const
{
    return m_uuid;
}

QString KeePass2::UuidNamePair::name() const
{
    return m_name;
}
