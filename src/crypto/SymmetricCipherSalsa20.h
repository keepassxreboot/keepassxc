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

#ifndef KEEPASSX_SYMMETRICCIPHERSALSA20_H
#define KEEPASSX_SYMMETRICCIPHERSALSA20_H

#include "crypto/SymmetricCipher.h"
#include "crypto/SymmetricCipherBackend.h"
#include "crypto/salsa20/ecrypt-sync.h"

class SymmetricCipherSalsa20 : public SymmetricCipherBackend
{
public:
    SymmetricCipherSalsa20(SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                           SymmetricCipher::Direction direction);
    ~SymmetricCipherSalsa20();
    void setAlgorithm(SymmetricCipher::Algorithm algo);
    void setMode(SymmetricCipher::Mode mode);
    void setDirection(SymmetricCipher::Direction direction);
    void setKey(const QByteArray& key);
    void setIv(const QByteArray& iv);

    QByteArray process(const QByteArray& data);
    void processInPlace(QByteArray& data);
    void processInPlace(QByteArray& data, quint64 rounds);

    void reset();
    int blockSize() const;

private:
    ECRYPT_ctx m_ctx;
    QByteArray m_key;
    QByteArray m_iv;
};

#endif // KEEPASSX_SYMMETRICCIPHERSALSA20_H
