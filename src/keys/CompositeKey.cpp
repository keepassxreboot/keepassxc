/*
*  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "CompositeKey.h"
#include "ChallengeResponseKey.h"

#include <QElapsedTimer>
#include <QFile>
#include <QtConcurrent>

#include "crypto/kdf/Kdf.h"
#include "format/KeePass2.h"
#include "core/Global.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

CompositeKey::CompositeKey()
{
}

CompositeKey::CompositeKey(const CompositeKey& key)
{
    *this = key;
}

CompositeKey::~CompositeKey()
{
    clear();
}

void CompositeKey::clear()
{
    qDeleteAll(m_keys);
    m_keys.clear();
    m_challengeResponseKeys.clear();
}

bool CompositeKey::isEmpty() const
{
    return m_keys.isEmpty() && m_challengeResponseKeys.isEmpty();
}

CompositeKey* CompositeKey::clone() const
{
    return new CompositeKey(*this);
}

CompositeKey& CompositeKey::operator=(const CompositeKey& key)
{
    // handle self assignment as that would break when calling clear()
    if (this == &key) {
        return *this;
    }

    clear();

    for (const Key* subKey : asConst(key.m_keys)) {
        addKey(*subKey);
    }
    for (const auto subKey : asConst(key.m_challengeResponseKeys)) {
        addChallengeResponseKey(subKey);
    }

    return *this;
}

QByteArray CompositeKey::rawKey() const
{
    CryptoHash cryptoHash(CryptoHash::Sha256);

    for (const Key* key : m_keys) {
        cryptoHash.addData(key->rawKey());
    }

    return cryptoHash.result();
}

bool CompositeKey::transform(const Kdf& kdf, QByteArray& result) const
{
    return kdf.transform(rawKey(), result);
}

bool CompositeKey::challenge(const QByteArray& seed, QByteArray& result) const
{
    // if no challenge response was requested, return nothing to
    // maintain backwards compatibility with regular databases.
    if (m_challengeResponseKeys.length() == 0) {
        result.clear();
        return true;
    }

    CryptoHash cryptoHash(CryptoHash::Sha256);

    for (const auto key : m_challengeResponseKeys) {
        // if the device isn't present or fails, return an error
        if (!key->challenge(seed)) {
            return false;
        }
        cryptoHash.addData(key->rawKey());
    }

    result = cryptoHash.result();
    return true;
}

void CompositeKey::addKey(const Key& key)
{
    m_keys.append(key.clone());
}

void CompositeKey::addChallengeResponseKey(QSharedPointer<ChallengeResponseKey> key)
{
    m_challengeResponseKeys.append(key);
}
