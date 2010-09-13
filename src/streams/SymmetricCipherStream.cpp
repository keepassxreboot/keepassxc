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

#include "SymmetricCipherStream.h"

SymmetricCipherStream::SymmetricCipherStream(QIODevice* baseDevice, SymmetricCipher::Algorithm algo, SymmetricCipher::Mode mode,
                      SymmetricCipher::Direction direction, const QByteArray& key, const QByteArray& iv)
                          : LayeredStream(baseDevice)
                          , m_bufferPos(0)
                          , m_eof(false)
{
    m_cipher = new SymmetricCipher(algo, mode, direction, key, iv);
}

bool SymmetricCipherStream::reset()
{
    m_buffer.clear();
    m_bufferPos = 0;
    m_cipher->reset();
    return true;
}

qint64 SymmetricCipherStream::readData(char* data, qint64 maxSize)
{
    // TODO m_eof is probably wrong and should be removed
    if (m_eof) {
        return 0;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        if (m_bufferPos == m_buffer.size()) {
            if (!readBlock()) {
                return maxSize - bytesRemaining;
            }
        }

        int bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(m_buffer.size() - m_bufferPos));

        memcpy(data + offset, m_buffer.constData() + m_bufferPos, bytesToCopy);

        offset += bytesToCopy;
        m_bufferPos += bytesToCopy;
        bytesRemaining -= bytesToCopy;
    }

    return maxSize;
}

bool SymmetricCipherStream::readBlock()
{
    m_buffer = m_baseDevice->read(m_cipher->blockSize());

    if (m_buffer.size() != m_cipher->blockSize()) {
        m_eof = true;
        // TODO check if m_buffer.size()!=0
        return false;
    }
    else {
        m_cipher->processInPlace(m_buffer);
        m_bufferPos = 0;
        return true;
    }
}

qint64 SymmetricCipherStream::writeData(const char* data, qint64 maxSize)
{
    // TODO implement
    return 0;
}

bool SymmetricCipherStream::writeBlock()
{
    // TODO implement
    return false;
}
