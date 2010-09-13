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

#include "HashedBlockStream.h"

#include <cstring>

#include "crypto/CryptoHash.h"

const QSysInfo::Endian HashedBlockStream::BYTEORDER = QSysInfo::LittleEndian;

HashedBlockStream::HashedBlockStream(QIODevice* baseDevice)
    : LayeredStream(baseDevice)
    , m_blockSize(1024*1024)
{
    init();
}

HashedBlockStream::HashedBlockStream(QIODevice* baseDevice, qint32 blockSize)
    : LayeredStream(baseDevice)
    , m_blockSize(blockSize)
{
    init();
}

void HashedBlockStream::init()
{
    m_bufferPos = 0;
    m_blockIndex = 0;
    m_eof = false;
}

void HashedBlockStream::close()
{
    LayeredStream::close();
}

qint64 HashedBlockStream::readData(char* data, qint64 maxSize)
{
    if (m_eof) {
        return 0;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        if (m_bufferPos == m_buffer.size()) {
            if (!readHashedBlock()) {
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

bool HashedBlockStream::readHashedBlock()
{
    bool ok;

    quint32 index = Endian::readUInt32(m_baseDevice, BYTEORDER, &ok);
    if (!ok || index != m_blockIndex) {
        // TODO error
        Q_ASSERT(false);
        return false;
    }

    QByteArray hash = m_baseDevice->read(32);
    if (hash.size() != 32) {
        // TODO error
        Q_ASSERT(false);
        return false;
    }

    m_blockSize = Endian::readInt32(m_baseDevice, BYTEORDER, &ok);
    if (!ok || m_blockSize < 0) {
        // TODO error
        Q_ASSERT(false);
        return false;
    }

    if (m_blockSize == 0) {
        if (hash.count(static_cast<char>(0)) != 32) {
            // TODO error
            Q_ASSERT(false);
            return false;
        }

        m_eof = true;
        return false;
    }

    m_buffer = m_baseDevice->read(m_blockSize);
    if (m_buffer.size() != m_blockSize) {
        // TODO error
        Q_ASSERT(false);
        return false;
    }

    if (hash != CryptoHash::hash(m_buffer, CryptoHash::Sha256)) {
        // TODO error
        Q_ASSERT(false);
        return false;
    }

    m_bufferPos = 0;
    m_blockIndex++;

    return true;
}

qint64 HashedBlockStream::writeData(const char* data, qint64 maxSize)
{
    // TODO implement
    return 0;
}

bool HashedBlockStream::writeHashedBlock()
{
    // TODO implement
    return false;
}
