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

SymmetricCipher::SymmetricCipher(Algorithm algo, Mode mode, Direction direction)
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

SymmetricCipherBackend* SymmetricCipher::createBackend(Algorithm algo, Mode mode, Direction direction)
{
    switch (algo) {
    case Aes128:
    case Aes256:
    case Twofish:
    case Salsa20:
    case ChaCha20:
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

int SymmetricCipher::keySize() const
{
    return m_backend->keySize();
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
        return Aes256;
    } else if (cipher == KeePass2::CIPHER_CHACHA20) {
        return ChaCha20;
    } else if (cipher == KeePass2::CIPHER_TWOFISH) {
        return Twofish;
    }

    qWarning("SymmetricCipher::cipherToAlgorithm: invalid Uuid %s", cipher.toByteArray().toHex().data());
    return InvalidAlgorithm;
}

Uuid SymmetricCipher::algorithmToCipher(Algorithm algo)
{
    switch (algo) {
    case Aes256:
        return KeePass2::CIPHER_AES;
    case ChaCha20:
        return KeePass2::CIPHER_CHACHA20;
    case Twofish:
        return KeePass2::CIPHER_TWOFISH;
    default:
        qWarning("SymmetricCipher::algorithmToCipher: invalid algorithm %d", algo);
        return Uuid();
    }
}

int SymmetricCipher::algorithmIvSize(Algorithm algo)
{
    switch (algo) {
    case ChaCha20:
        return 12;
    case Aes256:
        return 16;
    case Twofish:
        return 16;
    default:
        qWarning("SymmetricCipher::algorithmIvSize: invalid algorithm %d", algo);
        return -1;
    }
}

SymmetricCipher::Mode SymmetricCipher::algorithmMode(Algorithm algo)
{
    switch (algo) {
    case ChaCha20:
        return Stream;
    case Aes256:
    case Twofish:
        return Cbc;
    default:
        qWarning("SymmetricCipher::algorithmMode: invalid algorithm %d", algo);
        return InvalidMode;
    }
}

SymmetricCipher::Algorithm SymmetricCipher::algorithm() const
{
    return m_algo;
}
