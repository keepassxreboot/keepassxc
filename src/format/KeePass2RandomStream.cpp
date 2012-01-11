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

#include "KeePass2RandomStream.h"

#include "crypto/CryptoHash.h"
#include "format/KeePass2.h"

KeePass2RandomStream::KeePass2RandomStream(const QByteArray& key)
    : m_cipher(SymmetricCipher::Salsa20, SymmetricCipher::Stream, SymmetricCipher::Encrypt,
          CryptoHash::hash(key, CryptoHash::Sha256), KeePass2::INNER_STREAM_SALSA20_IV)
    , m_offset(0)
{
}

QByteArray KeePass2RandomStream::randomBytes(int size)
{
    QByteArray result;

    int bytesRemaining = size;

    while (bytesRemaining > 0) {
        if (m_buffer.size() == m_offset) {
            loadBlock();
        }

        int bytesToCopy = qMin(bytesRemaining, m_buffer.size() - m_offset);
        result.append(m_buffer.mid(m_offset, bytesToCopy));
        m_offset += bytesToCopy;
        bytesRemaining -= bytesToCopy;
    }

    return result;
}

QByteArray KeePass2RandomStream::process(const QByteArray& data)
{
    QByteArray randomData = randomBytes(data.size());
    QByteArray result;
    result.resize(data.size());

    for (int i=0; i<data.size(); i++) {
        result[i] = data[i] ^ randomData[i];
    }

    return result;
}

void KeePass2RandomStream::processInPlace(QByteArray& data)
{
    QByteArray randomData = randomBytes(data.size());

    for (int i=0; i<data.size(); i++) {
        data[i] = data[i] ^ randomData[i];
    }
}

void KeePass2RandomStream::loadBlock()
{
    Q_ASSERT(m_offset == m_buffer.size());

    m_buffer.fill('\0', m_cipher.blockSize());
    m_cipher.processInPlace(m_buffer);
    m_offset = 0;
}
