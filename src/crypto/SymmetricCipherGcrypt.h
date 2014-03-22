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

#ifndef KEEPASSX_SYMMETRICCIPHERGCRYPT_H
#define KEEPASSX_SYMMETRICCIPHERGCRYPT_H

#include <gcrypt.h>

#include "crypto/SymmetricCipher.h"
#include "crypto/SymmetricCipherBackend.h"

class SymmetricCipherGcrypt : public SymmetricCipherBackend
{
public:
    SymmetricCipherGcrypt(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                          SymmetricCipher::Direction direction);
    ~SymmetricCipherGcrypt();
    void setKey(const QByteArray& key);
    void setIv(const QByteArray& iv);

    QByteArray process(const QByteArray& data);
    void processInPlace(QByteArray& data);
    void processInPlace(QByteArray& data, quint64 rounds);

    void reset();
    int blockSize() const;

private:
    static int gcryptAlgo(SymmetricCipher::Algorithm algo);
    static int gcryptMode(SymmetricCipher::Mode mode);

    gcry_cipher_hd_t m_ctx;
    const int m_algo;
    const int m_mode;
    const SymmetricCipher::Direction m_direction;
    QByteArray m_key;
    QByteArray m_iv;
    int m_blockSize;
};

#endif // KEEPASSX_SYMMETRICCIPHERGCRYPT_H
