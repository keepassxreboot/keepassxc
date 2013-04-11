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

#include "SymmetricCipher.h"

#include "crypto/SymmetricCipherGcrypt.h"
#include "crypto/SymmetricCipherSalsa20.h"

SymmetricCipher::SymmetricCipher()
    : m_backend(0)
{
}

SymmetricCipher::SymmetricCipher(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                 SymmetricCipher::Direction direction, const QByteArray& key, const QByteArray& iv)
    : m_backend(0)
{
    init(algo, mode, direction, key, iv);
}

SymmetricCipher::~SymmetricCipher()
{
}

SymmetricCipherBackend* SymmetricCipher::createBackend(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                                       SymmetricCipher::Direction direction)
{
    switch (algo) {
    case SymmetricCipher::Aes256:
    case SymmetricCipher::Twofish:
        return new SymmetricCipherGcrypt(algo, mode, direction);

    case SymmetricCipher::Salsa20:
        return new SymmetricCipherSalsa20(algo, mode, direction);

    default:
        Q_ASSERT(false);
        return Q_NULLPTR;
    }
}

void SymmetricCipher::init(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                           SymmetricCipher::Direction direction, const QByteArray& key, const QByteArray& iv)
{
    Q_ASSERT(!m_backend);
    m_backend.reset(createBackend(algo, mode, direction));
    m_backend->init();
    m_backend->setKey(key);
    if (!iv.isNull())
        m_backend->setIv(iv);
}

void SymmetricCipher::reset()
{
    Q_ASSERT(m_backend);
    m_backend->reset();
}

void SymmetricCipher::setIv(const QByteArray &iv)
{
    Q_ASSERT(m_backend);
    m_backend->setIv(iv);
}

int SymmetricCipher::blockSize() const
{
    Q_ASSERT(m_backend);
    return m_backend->blockSize();
}
