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

#include "config-keepassx.h"
#include "crypto/SymmetricCipherGcrypt.h"

SymmetricCipher::SymmetricCipher(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                 SymmetricCipher::Direction direction)
    : m_backend(createBackend(algo, mode, direction))
    , m_initialized(false)
{
}

SymmetricCipher::~SymmetricCipher()
{
}

bool SymmetricCipher::init(const QByteArray& key, const QByteArray& iv)
{
    if (!m_backend->init()) {
        return false;
    }

    if (!m_backend->setKey(key)) {
        return false;
    }

    if (!m_backend->setIv(iv)) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool SymmetricCipher::isInitalized() const
{
    return m_initialized;
}

SymmetricCipherBackend* SymmetricCipher::createBackend(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                                       SymmetricCipher::Direction direction)
{
    switch (algo) {
    case SymmetricCipher::Aes256:
    case SymmetricCipher::Twofish:
    case SymmetricCipher::Salsa20:
        return new SymmetricCipherGcrypt(algo, mode, direction);

    default:
        Q_ASSERT(false);
        return nullptr;
    }
}

bool SymmetricCipher::reset()
{
    return m_backend->reset();
}

int SymmetricCipher::blockSize() const
{
    return m_backend->blockSize();
}

QString SymmetricCipher::errorString() const
{
    return m_backend->errorString();
}
