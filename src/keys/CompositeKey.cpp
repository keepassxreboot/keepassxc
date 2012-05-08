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
#include "CompositeKey_p.h"

#include <QtCore/QtConcurrentRun>
#include <QtCore/QTime>

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
    clear();
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
    clear();

    Q_FOREACH (const Key* subKey, key.m_keys) {
        addKey(*subKey);
    }

    return *this;
}

QByteArray CompositeKey::rawKey() const
{
    CryptoHash cryptoHash(CryptoHash::Sha256);

    Q_FOREACH (const Key* key, m_keys) {
        cryptoHash.addData(key->rawKey());
    }

    return cryptoHash.result();
}

QByteArray CompositeKey::transform(const QByteArray& seed, quint64 rounds) const
{
    Q_ASSERT(seed.size() == 32);
    Q_ASSERT(rounds > 0);

    QByteArray key = rawKey();

    QFuture<QByteArray> future = QtConcurrent::run(transformKeyRaw, key.left(16), seed, rounds);
    QByteArray result2 = transformKeyRaw(key.right(16), seed, rounds);

    QByteArray transformed;
    transformed.append(future.result());
    transformed.append(result2);

    return CryptoHash::hash(transformed, CryptoHash::Sha256);
}

QByteArray CompositeKey::transformKeyRaw(const QByteArray& key, const QByteArray& seed,
                                         quint64 rounds) {
    QByteArray iv(16, 0);
    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt, seed, iv);

    QByteArray result = key;

    cipher.processInPlace(result, rounds);

    return result;
}

void CompositeKey::addKey(const Key& key)
{
    m_keys.append(key.clone());
}

int CompositeKey::transformKeyBenchmark(int msec)
{
    TransformKeyBenchmarkThread thread1(msec);
    TransformKeyBenchmarkThread thread2(msec);

    thread1.start();
    thread2.start();

    thread1.wait();
    thread2.wait();

    return qMin(thread1.rounds(), thread2.rounds());
}


TransformKeyBenchmarkThread::TransformKeyBenchmarkThread(int msec)
    : m_msec(msec)
    , m_rounds(0)
{
    Q_ASSERT(msec > 0);
}

int TransformKeyBenchmarkThread::rounds()
{
    return m_rounds;
}

void TransformKeyBenchmarkThread::run()
{
    QByteArray key = QByteArray('\x7E', 32);
    QByteArray seed = QByteArray('\x4B', 32);
    QByteArray iv(16, 0);

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt, seed, iv);

    QTime t;
    t.start();

    do {
        cipher.processInPlace(key, 100);
        m_rounds += 100;
    } while (t.elapsed() < m_msec);
}
