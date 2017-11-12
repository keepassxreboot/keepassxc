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
#include "core/Uuid.h"

const Uuid KeePass2::CIPHER_AES = Uuid(QByteArray::fromHex("31c1f2e6bf714350be5805216afc5aff"));
const Uuid KeePass2::CIPHER_TWOFISH = Uuid(QByteArray::fromHex("ad68f29f576f4bb9a36ad47af965346c"));

const Uuid KeePass2::KDF_AES = Uuid(QByteArray::fromHex("C9D9F39A628A4460BF740D08C18A4FEA"));

const QByteArray KeePass2::INNER_STREAM_SALSA20_IV("\xE8\x30\x09\x4B\x97\x20\x5D\x2A");

const QList<KeePass2::UuidNamePair> KeePass2::CIPHERS {
        KeePass2::UuidNamePair(KeePass2::CIPHER_AES, "AES: 256-bit"),
        KeePass2::UuidNamePair(KeePass2::CIPHER_TWOFISH, "Twofish: 256-bit"),
};
const QList<KeePass2::UuidNamePair> KeePass2::KDFS {
        KeePass2::UuidNamePair(KeePass2::KDF_AES, "AES-KDF"),
};

Kdf* KeePass2::uuidToKdf(const Uuid& uuid) {
    if (uuid == KDF_AES) {
        return static_cast<Kdf*>(new AesKdf());
    }

    return nullptr;
}

Uuid KeePass2::kdfToUuid(const Kdf& kdf)
{
    switch (kdf.type()) {
        case Kdf::Type::AES:
            return KDF_AES;
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
