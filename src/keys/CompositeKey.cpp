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

#include <QtConcurrent>
#include <QElapsedTimer>

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

bool CompositeKey::isEmpty() const
{
    return m_keys.isEmpty();
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

QByteArray CompositeKey::transform(const QByteArray& seed, quint64 rounds,
                                   bool* ok, QString* errorString) const
{
    Q_ASSERT(seed.size() == 32);
    Q_ASSERT(rounds > 0);

    bool okLeft;
    QString errorStringLeft;
    bool okRight;
    QString errorStringRight;

    QByteArray key = rawKey();

    QFuture<QByteArray> future = QtConcurrent::run(transformKeyRaw, key.left(16), seed, rounds,
                                                   &okLeft, &errorStringLeft);
    QByteArray result2 = transformKeyRaw(key.right(16), seed, rounds, &okRight, &errorStringRight);

    QByteArray transformed;
    transformed.append(future.result());
    transformed.append(result2);

    *ok = (okLeft && okRight);

    if (!okLeft) {
        *errorString = errorStringLeft;
        return QByteArray();
    }

    if (!okRight) {
        *errorString = errorStringRight;
        return QByteArray();
    }

    return CryptoHash::hash(transformed, CryptoHash::Sha256);
}

QByteArray CompositeKey::transformKeyRaw(const QByteArray& key, const QByteArray& seed,
                                         quint64 rounds, bool* ok, QString* errorString)
{
    QByteArray iv(16, 0);
    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt);
    if (!cipher.init(seed, iv)) {
        *ok = false;
        *errorString = cipher.errorString();
        return QByteArray();
    }

    QByteArray result = key;

    if (!cipher.processInPlace(result, rounds)) {
        *ok = false;
        *errorString = cipher.errorString();
        return QByteArray();
    }

    *ok = true;
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
    QByteArray key = QByteArray(16, '\x7E');
    QByteArray seed = QByteArray(32, '\x4B');
    QByteArray iv(16, 0);

    SymmetricCipher cipher(SymmetricCipher::Aes256, SymmetricCipher::Ecb,
                           SymmetricCipher::Encrypt);
    cipher.init(seed, iv);

    QElapsedTimer t;
    t.start();

    do {
        if (!cipher.processInPlace(key, 10000)) {
            m_rounds = -1;
            return;
        }
        m_rounds += 10000;
    } while (!t.hasExpired(m_msec));
}
