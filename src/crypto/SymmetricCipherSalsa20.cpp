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

#include "SymmetricCipherSalsa20.h"

SymmetricCipherSalsa20::SymmetricCipherSalsa20(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                       SymmetricCipher::Direction direction)
{
    Q_ASSERT(algo == SymmetricCipher::Salsa20);
    Q_UNUSED(algo);

    Q_ASSERT(mode == SymmetricCipher::Stream);
    Q_UNUSED(mode);

    Q_UNUSED(direction);
}

SymmetricCipherSalsa20::~SymmetricCipherSalsa20()
{
}

bool SymmetricCipherSalsa20::init()
{
    return true;
}

bool SymmetricCipherSalsa20::setKey(const QByteArray& key)
{
    Q_ASSERT((key.size() == 16) || (key.size() == 32));

    m_key = key;
    ECRYPT_keysetup(&m_ctx, reinterpret_cast<const u8*>(m_key.constData()), m_key.size()*8, 64);

    return true;
}

bool SymmetricCipherSalsa20::setIv(const QByteArray& iv)
{
    Q_ASSERT(iv.size() == 8);

    m_iv = iv;
    ECRYPT_ivsetup(&m_ctx, reinterpret_cast<const u8*>(m_iv.constData()));

    return true;
}

QByteArray SymmetricCipherSalsa20::process(const QByteArray& data, bool* ok)
{
    Q_ASSERT((data.size() < blockSize()) || ((data.size() % blockSize()) == 0));

    QByteArray result;
    result.resize(data.size());

    ECRYPT_encrypt_bytes(&m_ctx, reinterpret_cast<const u8*>(data.constData()),
                         reinterpret_cast<u8*>(result.data()), data.size());

    *ok = true;
    return result;
}

bool SymmetricCipherSalsa20::processInPlace(QByteArray& data)
{
    Q_ASSERT((data.size() < blockSize()) || ((data.size() % blockSize()) == 0));

    ECRYPT_encrypt_bytes(&m_ctx, reinterpret_cast<const u8*>(data.constData()),
                         reinterpret_cast<u8*>(data.data()), data.size());

    return true;
}

bool SymmetricCipherSalsa20::processInPlace(QByteArray& data, quint64 rounds)
{
    Q_ASSERT((data.size() < blockSize()) || ((data.size() % blockSize()) == 0));

    for (quint64 i = 0; i != rounds; ++i) {
        ECRYPT_encrypt_bytes(&m_ctx, reinterpret_cast<const u8*>(data.constData()),
                             reinterpret_cast<u8*>(data.data()), data.size());
    }

    return true;
}

bool SymmetricCipherSalsa20::reset()
{
    ECRYPT_ivsetup(&m_ctx, reinterpret_cast<const u8*>(m_iv.constData()));

    return true;
}

int SymmetricCipherSalsa20::blockSize() const
{
    return 64;
}

QString SymmetricCipherSalsa20::errorString() const
{
    return QString();
}
