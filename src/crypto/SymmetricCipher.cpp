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

#include <gcrypt.h>

class SymmetricCipherPrivate
{
public:
    gcry_cipher_hd_t ctx;
    SymmetricCipher::Direction direction;
    QByteArray key;
    QByteArray iv;
    int blockSize;
};

SymmetricCipher::SymmetricCipher(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                                 SymmetricCipher::Direction direction, const QByteArray& key, const QByteArray& iv)
                                     : d_ptr(new SymmetricCipherPrivate())
{
    Q_D(SymmetricCipher);

    d->direction = direction;
    d->key = key;
    d->iv = iv;

    int algoGcrypt;

    switch (algo) {
    case SymmetricCipher::Aes256:
        algoGcrypt = GCRY_CIPHER_AES256;
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    int modeGcrypt;

    switch (mode) {
    case SymmetricCipher::Ecb:
        modeGcrypt = GCRY_CIPHER_MODE_ECB;
        break;

    case SymmetricCipher::Cbc:
        modeGcrypt = GCRY_CIPHER_MODE_CBC;
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    gcry_error_t error;

    error = gcry_cipher_open(&d->ctx, algoGcrypt, modeGcrypt, 0);
    Q_ASSERT(error == 0); // TODO real error checking
    error = gcry_cipher_setkey(d->ctx, d->key.constData(), d->key.size());
    Q_ASSERT(error == 0);
    error = gcry_cipher_setiv(d->ctx, d->iv.constData(), d->iv.size());
    Q_ASSERT(error == 0);

    size_t blockSizeT;
    error = gcry_cipher_algo_info(algoGcrypt, GCRYCTL_GET_BLKLEN, 0, &blockSizeT);
    Q_ASSERT(error == 0);
    d->blockSize = blockSizeT;
}

SymmetricCipher::~SymmetricCipher()
{
    Q_D(SymmetricCipher);

    gcry_cipher_close(d->ctx);

    delete d_ptr;
}

QByteArray SymmetricCipher::process(const QByteArray& data)
{
    Q_D(SymmetricCipher);

    // TODO check block size

    QByteArray result;
    result.resize(data.size());

    gcry_error_t error;

    if (d->direction == SymmetricCipher::Decrypt) {
        error = gcry_cipher_decrypt(d->ctx, result.data(), data.size(), data.constData(), data.size());
    }
    else {
        error = gcry_cipher_encrypt(d->ctx, result.data(), data.size(), data.constData(), data.size());
    }

    Q_ASSERT(error == 0);

    return result;
}

void SymmetricCipher::processInPlace(QByteArray& data)
{
    Q_D(SymmetricCipher);

    // TODO check block size

    gcry_error_t error;

    if (d->direction == SymmetricCipher::Decrypt) {
        error = gcry_cipher_decrypt(d->ctx, data.data(), data.size(), 0, 0);
    }
    else {
        error = gcry_cipher_encrypt(d->ctx, data.data(), data.size(), 0, 0);
    }

    Q_ASSERT(error == 0);
}

void SymmetricCipher::reset()
{
    Q_D(SymmetricCipher);

    gcry_error_t error;

    error = gcry_cipher_reset(d->ctx);
    Q_ASSERT(error == 0);
    error = gcry_cipher_setiv(d->ctx, d->iv.constData(), d->iv.size());
    Q_ASSERT(error == 0);
}

int SymmetricCipher::blockSize() const
{
    Q_D(const SymmetricCipher);

    return d->blockSize;
}
