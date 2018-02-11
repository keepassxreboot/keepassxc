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

#include "SymmetricCipherGcrypt.h"

#include "config-keepassx.h"
#include "crypto/Crypto.h"

SymmetricCipherGcrypt::SymmetricCipherGcrypt(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                             SymmetricCipher::Direction direction)
    : m_ctx(nullptr)
    , m_algo(gcryptAlgo(algo))
    , m_mode(gcryptMode(mode))
    , m_direction(direction)
{
}

SymmetricCipherGcrypt::~SymmetricCipherGcrypt()
{
    gcry_cipher_close(m_ctx);
}

int SymmetricCipherGcrypt::gcryptAlgo(SymmetricCipher::Algorithm algo)
{
    switch (algo) {
    case SymmetricCipher::Aes128:
        return GCRY_CIPHER_AES128;

    case SymmetricCipher::Aes256:
        return GCRY_CIPHER_AES256;

    case SymmetricCipher::Twofish:
        return GCRY_CIPHER_TWOFISH;

    case SymmetricCipher::Salsa20:
        return GCRY_CIPHER_SALSA20;

    case SymmetricCipher::ChaCha20:
        return GCRY_CIPHER_CHACHA20;

    default:
        Q_ASSERT(false);
        return -1;
    }
}

int SymmetricCipherGcrypt::gcryptMode(SymmetricCipher::Mode mode)
{
    switch (mode) {
    case SymmetricCipher::Ecb:
        return GCRY_CIPHER_MODE_ECB;

    case SymmetricCipher::Cbc:
        return GCRY_CIPHER_MODE_CBC;

    case SymmetricCipher::Ctr:
        return GCRY_CIPHER_MODE_CTR;

    case SymmetricCipher::Stream:
        return GCRY_CIPHER_MODE_STREAM;

    default:
        Q_ASSERT(false);
        return -1;
    }
}

void SymmetricCipherGcrypt::setErrorString(gcry_error_t err)
{
    const char* gcryptError = gcry_strerror(err);
    const char* gcryptErrorSource = gcry_strsource(err);

    m_errorString = QString("%1/%2").arg(QString::fromLocal8Bit(gcryptErrorSource),
                                         QString::fromLocal8Bit(gcryptError));
}

bool SymmetricCipherGcrypt::init()
{
    Q_ASSERT(Crypto::initalized());

    gcry_error_t error;

    if(m_ctx != nullptr)
        gcry_cipher_close(m_ctx);
    error = gcry_cipher_open(&m_ctx, m_algo, m_mode, 0);
    if (error != 0) {
        setErrorString(error);
        return false;
    }

    return true;
}

bool SymmetricCipherGcrypt::setKey(const QByteArray& key)
{
    m_key = key;
    gcry_error_t error = gcry_cipher_setkey(m_ctx, m_key.constData(), m_key.size());

    if (error != 0) {
        setErrorString(error);
        return false;
    }

    return true;
}

bool SymmetricCipherGcrypt::setIv(const QByteArray& iv)
{
    m_iv = iv;
    gcry_error_t error;

    if (m_mode == GCRY_CIPHER_MODE_CTR) {
        error = gcry_cipher_setctr(m_ctx, m_iv.constData(), m_iv.size());
    } else {
        error = gcry_cipher_setiv(m_ctx, m_iv.constData(), m_iv.size());
    }

    if (error != 0) {
        setErrorString(error);
        return false;
    }

    return true;
}

QByteArray SymmetricCipherGcrypt::process(const QByteArray& data, bool* ok)
{
    // TODO: check block size

    QByteArray result;
    result.resize(data.size());

    gcry_error_t error;

    if (m_direction == SymmetricCipher::Decrypt) {
        error = gcry_cipher_decrypt(m_ctx, result.data(), data.size(), data.constData(), data.size());
    } else {
        error = gcry_cipher_encrypt(m_ctx, result.data(), data.size(), data.constData(), data.size());
    }

    if (error != 0) {
        setErrorString(error);
        *ok = false;
    } else {
        *ok = true;
    }

    return result;
}

bool SymmetricCipherGcrypt::processInPlace(QByteArray& data)
{
    // TODO: check block size

    gcry_error_t error;

    if (m_direction == SymmetricCipher::Decrypt) {
        error = gcry_cipher_decrypt(m_ctx, data.data(), data.size(), nullptr, 0);
    } else {
        error = gcry_cipher_encrypt(m_ctx, data.data(), data.size(), nullptr, 0);
    }

    if (error != 0) {
        setErrorString(error);
        return false;
    }

    return true;
}

bool SymmetricCipherGcrypt::processInPlace(QByteArray& data, quint64 rounds)
{
    // TODO: check block size

    gcry_error_t error;

    char* rawData = data.data();
    int size = data.size();

    if (m_direction == SymmetricCipher::Decrypt) {
        for (quint64 i = 0; i != rounds; ++i) {
            error = gcry_cipher_decrypt(m_ctx, rawData, size, nullptr, 0);

            if (error != 0) {
                setErrorString(error);
                return false;
            }
        }
    } else {
        for (quint64 i = 0; i != rounds; ++i) {
            error = gcry_cipher_encrypt(m_ctx, rawData, size, nullptr, 0);

            if (error != 0) {
                setErrorString(error);
                return false;
            }
        }
    }

    return true;
}

bool SymmetricCipherGcrypt::reset()
{
    gcry_error_t error;

    error = gcry_cipher_reset(m_ctx);
    if (error != 0) {
        setErrorString(error);
        return false;
    }

    error = gcry_cipher_setiv(m_ctx, m_iv.constData(), m_iv.size());
    if (error != 0) {
        setErrorString(error);
        return false;
    }

    return true;
}

int SymmetricCipherGcrypt::keySize() const
{
    gcry_error_t error;
    size_t keySizeT;

    error = gcry_cipher_algo_info(m_algo, GCRYCTL_GET_KEYLEN, nullptr, &keySizeT);
    if (error != 0)
        return -1;

    return keySizeT;
}

int SymmetricCipherGcrypt::blockSize() const
{
    gcry_error_t error;
    size_t blockSizeT;

    error = gcry_cipher_algo_info(m_algo, GCRYCTL_GET_BLKLEN, nullptr, &blockSizeT);
    if (error != 0)
        return -1;

    return blockSizeT;
}

QString SymmetricCipherGcrypt::errorString() const
{
    return m_errorString;
}
