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

#include "CryptoHash.h"

#include <gcrypt.h>

#include "crypto/Crypto.h"

class CryptoHashPrivate
{
public:
    gcry_md_hd_t ctx;
    int hashLen;
};

CryptoHash::CryptoHash(CryptoHash::Algorithm algo)
    : CryptoHash::CryptoHash(algo, false) {}

CryptoHash::CryptoHash(CryptoHash::Algorithm algo, bool hmac)
    : d_ptr(new CryptoHashPrivate())
{
    Q_D(CryptoHash);

    Q_ASSERT(Crypto::initalized());

    int algoGcrypt = -1;
    unsigned int flagsGcrypt = 0;

    switch (algo) {
    case CryptoHash::Sha256:
        algoGcrypt = GCRY_MD_SHA256;
        break;

    case CryptoHash::Sha512:
        algoGcrypt = GCRY_MD_SHA512;
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    if (hmac) {
        flagsGcrypt |= GCRY_MD_FLAG_HMAC;
    }

    gcry_error_t error = gcry_md_open(&d->ctx, algoGcrypt, flagsGcrypt);
    if (error) {
        qWarning("Gcrypt error (ctor): %s", gcry_strerror(error));
        qWarning("Gcrypt error (ctor): %s", gcry_strsource(error));
    }
    Q_ASSERT(error == 0); // TODO: error handling

    d->hashLen = gcry_md_get_algo_dlen(algoGcrypt);
}

CryptoHash::~CryptoHash()
{
    Q_D(CryptoHash);

    gcry_md_close(d->ctx);

    delete d_ptr;
}

void CryptoHash::addData(const QByteArray& data)
{
    Q_D(CryptoHash);

    if (data.isEmpty()) {
        return;
    }

    gcry_md_write(d->ctx, data.constData(), data.size());
}

void CryptoHash::setKey(const QByteArray& data)
{
    Q_D(CryptoHash);

    gcry_error_t error = gcry_md_setkey(d->ctx, data.constData(), data.size());
    if (error) {
        qWarning("Gcrypt error (setKey): %s", gcry_strerror(error));
        qWarning("Gcrypt error (setKey): %s", gcry_strsource(error));
    }
    Q_ASSERT(error == 0);
}

void CryptoHash::reset()
{
    Q_D(CryptoHash);

    gcry_md_reset(d->ctx);
}

QByteArray CryptoHash::result() const
{
    Q_D(const CryptoHash);

    const char* result = reinterpret_cast<const char*>(gcry_md_read(d->ctx, 0));
    return QByteArray(result, d->hashLen);
}

QByteArray CryptoHash::hash(const QByteArray& data, CryptoHash::Algorithm algo)
{
    // replace with gcry_md_hash_buffer()?
    CryptoHash cryptoHash(algo);
    cryptoHash.addData(data);
    return cryptoHash.result();
}

QByteArray CryptoHash::hmac(const QByteArray& data, const QByteArray& key, CryptoHash::Algorithm algo)
{
    // replace with gcry_md_hash_buffer()?
    CryptoHash cryptoHash(algo, true);
    cryptoHash.setKey(key);
    cryptoHash.addData(data);
    return cryptoHash.result();
}
