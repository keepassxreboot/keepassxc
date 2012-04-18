/*
*  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QtCore/QtConcurrentRun>

#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"

CompositeKey::CompositeKey()
{
}

CompositeKey::CompositeKey(const CompositeKey& key)
{
    *this = key;
}

CompositeKey::~CompositeKey()
{
    qDeleteAll(m_keys);
}

void CompositeKey::clear()
{
    qDeleteAll(m_keys);
    m_keys.clear();
}

CompositeKey* CompositeKey::clone() const
{
    return new CompositeKey(*this);
}

CompositeKey& CompositeKey::operator=(const CompositeKey& key)
{
    Q_FOREACH (Key* subKey, key.m_keys) {
        m_keys.append(subKey->clone());
    }

    return *this;
}

QByteArray CompositeKey::rawKey() const
{
    CryptoHash cryptoHash(CryptoHash::Sha256);

    Q_FOREACH (Key* key, m_keys) {
        cryptoHash.addData(key->rawKey());
    }

    return cryptoHash.result();
}

QByteArray CompositeKey::transform(const QByteArray& seed, int rounds) const
{
    Q_ASSERT(seed.size() == 32);
    Q_ASSERT(rounds > 0);

    QByteArray key = rawKey();

    QFuture<QByteArray> future1 = QtConcurrent::run(transformKeyRaw, key.left(16), seed, rounds);
    QFuture<QByteArray> future2 = QtConcurrent::run(transformKeyRaw, key.right(16), seed, rounds);

    QByteArray transformed;
    transformed.append(future1.result());
    transformed.append(future2.result());

    return CryptoHash::hash(transformed, CryptoHash::Sha256);
}

QByteArray CompositeKey::transformKeyRaw(const QByteArray& key, const QByteArray& seed, int rounds) {
    QByteArray iv(16, 0);
    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb, SymmetricCipher::Encrypt, seed, iv);

    QByteArray result = key;

    for (int i = 0; i < rounds; i++) {
        cipher.processInPlace(result);
    }

    return result;
}

void CompositeKey::addKey(const Key& key)
{
    m_keys.append(key.clone());
}
