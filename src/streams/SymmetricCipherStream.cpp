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
                          , m_bufferFilling(false)
                          , m_error(false)
{
    m_cipher = new SymmetricCipher(algo, mode, direction, key, iv);
}

bool SymmetricCipherStream::reset()
{
    if (isWritable()) {
        if (!writeBlock()) {
            return false;
        }
    }

    m_buffer.clear();
    m_bufferPos = 0;
    m_bufferFilling = false;
    m_error = false;
    m_cipher->reset();

    return true;
}

void SymmetricCipherStream::close()
{
    if (isWritable()) {
        writeBlock();
    }

    LayeredStream::close();
}

qint64 SymmetricCipherStream::readData(char* data, qint64 maxSize)
{
    Q_ASSERT(maxSize >= 0);

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        if ((m_bufferPos == m_buffer.size()) || m_bufferFilling) {
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
    if (m_bufferFilling) {
        m_buffer.append(m_baseDevice->read(m_cipher->blockSize() - m_buffer.size()));
    }
    else {
        m_buffer = m_baseDevice->read(m_cipher->blockSize());
    }

    if (m_buffer.size() != m_cipher->blockSize()) {
        m_bufferFilling = true;
        return false;
    }
    else {
        m_cipher->processInPlace(m_buffer);
        m_bufferPos = 0;
        m_bufferFilling = false;
        return true;
    }
}

qint64 SymmetricCipherStream::writeData(const char* data, qint64 maxSize)
{
    Q_ASSERT(maxSize >= 0);

    if (m_error) {
        return 0;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        int bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(m_cipher->blockSize() - m_buffer.size()));

        m_buffer.append(data + offset, bytesToCopy);

        offset += bytesToCopy;
        bytesRemaining -= bytesToCopy;

        if (m_buffer.size() == m_cipher->blockSize()) {
            if (!writeBlock()) {
                if (m_error) {
                    return -1;
                }
                else {
                    return maxSize - bytesRemaining;
                }
            }
        }
    }

    return maxSize;
}

bool SymmetricCipherStream::writeBlock()
{
    if (m_buffer.isEmpty()) {
        return true;
    }
    else if (m_buffer.size() != m_cipher->blockSize()) {
        // PKCS7 padding
        int padLen = m_cipher->blockSize() - m_buffer.size();
        for (int i=m_buffer.size(); i<m_cipher->blockSize(); i++) {
            m_buffer.append(static_cast<char>(padLen));
        }
    }

    m_cipher->processInPlace(m_buffer);

    if (m_baseDevice->write(m_buffer) != m_buffer.size()) {
        m_error = true;
        // TODO copy error string
        return false;
    }
    else {
        m_buffer.clear();
        return true;
    }
}
