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

#include <QtCore/QThread>

#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"

class KeyTransformation : public QThread
{
    Q_OBJECT

public:
    KeyTransformation(const QByteArray& key, const QByteArray& seed, int rounds);
    QByteArray result();

protected:
    void run();

private:
    QByteArray m_key;
    QByteArray m_seed;
    int m_rounds;
    QByteArray m_result;
};

CompositeKey::~CompositeKey()
{
    qDeleteAll(m_keys);
}

CompositeKey* CompositeKey::clone() const
{
    return new CompositeKey(*this);
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
    QByteArray key = rawKey();

    KeyTransformation* transform1 = new KeyTransformation(key.left(16), seed, rounds);
    KeyTransformation* transform2 = new KeyTransformation(key.right(16), seed, rounds);

    transform1->start();
    transform2->start();

    transform1->wait();
    transform2->wait();

    QByteArray transformed;
    transformed.append(transform1->result());
    transformed.append(transform2->result());

    return CryptoHash::hash(transformed, CryptoHash::Sha256);
}

void CompositeKey::addKey(const Key& key)
{
    m_keys.append(key.clone());
}

KeyTransformation::KeyTransformation(const QByteArray& key, const QByteArray& seed, int rounds)
    : m_key(key)
    , m_seed(seed)
    , m_rounds(rounds)
    , m_result(key)
{
}

void KeyTransformation::run()
{
    QByteArray iv(16, 0);
    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb, SymmetricCipher::Encrypt, m_seed, iv);

    for (int i=0; i<m_rounds; i++) {
        cipher.processInPlace(m_result);
    }
}

QByteArray KeyTransformation::result()
{
    return m_result;
}

#include "KeyTransformation.moc"
