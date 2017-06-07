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
    , m_algo(algo)
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
    case SymmetricCipher::ChaCha20:
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

SymmetricCipher::Algorithm SymmetricCipher::cipherToAlgorithm(Uuid cipher)
{
    if (cipher == KeePass2::CIPHER_AES) {
        return SymmetricCipher::Aes256;
    } else if (cipher == KeePass2::CIPHER_CHACHA20) {
        return SymmetricCipher::ChaCha20;
    } else if (cipher == KeePass2::CIPHER_TWOFISH) {
        return SymmetricCipher::Twofish;
    }

    qWarning("SymmetricCipher::cipherToAlgorithm: invalid Uuid %s", cipher.toByteArray().toHex().data());
    return InvalidAlgorithm;
}

Uuid SymmetricCipher::algorithmToCipher(SymmetricCipher::Algorithm algo)
{
    switch (algo) {
    case SymmetricCipher::Aes256:
        return KeePass2::CIPHER_AES;
    case SymmetricCipher::ChaCha20:
        return KeePass2::CIPHER_CHACHA20;
    case SymmetricCipher::Twofish:
        return KeePass2::CIPHER_TWOFISH;
    default:
        qWarning("SymmetricCipher::algorithmToCipher: invalid algorithm %d", algo);
        return Uuid();
    }
}

int SymmetricCipher::algorithmIvSize(SymmetricCipher::Algorithm algo) {
    switch (algo) {
    case SymmetricCipher::ChaCha20:
        return 12;
    case SymmetricCipher::Aes256:
    case SymmetricCipher::Twofish:
        return 16;
    default:
        qWarning("SymmetricCipher::algorithmIvSize: invalid algorithm %d", algo);
        return -1;
    }
}

SymmetricCipher::Mode SymmetricCipher::algorithmMode(SymmetricCipher::Algorithm algo) {
    switch (algo) {
    case SymmetricCipher::ChaCha20:
        return SymmetricCipher::Stream;
    case SymmetricCipher::Aes256:
    case SymmetricCipher::Twofish:
        return SymmetricCipher::Cbc;
    default:
        qWarning("SymmetricCipher::algorithmMode: invalid algorithm %d", algo);
        return SymmetricCipher::InvalidMode;
    }
}

SymmetricCipher::Algorithm SymmetricCipher::algorithm() const {
    return m_algo;
}
