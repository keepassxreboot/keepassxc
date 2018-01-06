/*
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "HmacBlockStream.h"

#include "core/Endian.h"
#include "crypto/CryptoHash.h"

const QSysInfo::Endian HmacBlockStream::ByteOrder = QSysInfo::LittleEndian;

HmacBlockStream::HmacBlockStream(QIODevice* baseDevice, QByteArray key)
    : LayeredStream(baseDevice)
    , m_blockSize(1024 * 1024)
    , m_key(key)
{
    init();
}

HmacBlockStream::HmacBlockStream(QIODevice* baseDevice, QByteArray key, qint32 blockSize)
    : LayeredStream(baseDevice)
    , m_blockSize(blockSize)
    , m_key(key)
{
    init();
}

HmacBlockStream::~HmacBlockStream()
{
    close();
}

void HmacBlockStream::init()
{
    m_buffer.clear();
    m_bufferPos = 0;
    m_blockIndex = 0;
    m_eof = false;
    m_error = false;
}

bool HmacBlockStream::reset()
{
    // Write final block(s) only if device is writable and we haven't
    // already written a final block.
    if (isWritable() && (!m_buffer.isEmpty() || m_blockIndex != 0)) {
        if (!m_buffer.isEmpty() && !writeHashedBlock()) {
            return false;
        }

        // write empty final block
        if (!writeHashedBlock()) {
            return false;
        }
    }

    init();

    return true;
}

void HmacBlockStream::close()
{
    // Write final block(s) only if device is writable and we haven't
    // already written a final block.
    if (isWritable() && (!m_buffer.isEmpty() || m_blockIndex != 0)) {
        if (!m_buffer.isEmpty()) {
            writeHashedBlock();
        }

        // write empty final block
        writeHashedBlock();
    }

    LayeredStream::close();
}

qint64 HmacBlockStream::readData(char* data, qint64 maxSize)
{
    if (m_error) {
        return -1;
    } else if (m_eof) {
        return 0;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        if (m_bufferPos == m_buffer.size()) {
            if (!readHashedBlock()) {
                if (m_error) {
                    return -1;
                }
                return maxSize - bytesRemaining;
            }
        }

        qint64 bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(m_buffer.size() - m_bufferPos));

        memcpy(data + offset, m_buffer.constData() + m_bufferPos, static_cast<size_t>(bytesToCopy));

        offset += bytesToCopy;
        m_bufferPos += bytesToCopy;
        bytesRemaining -= bytesToCopy;
    }

    return maxSize;
}

bool HmacBlockStream::readHashedBlock()
{
    if (m_eof) {
        return false;
    }
    QByteArray hmac = m_baseDevice->read(32);
    if (hmac.size() != 32) {
        m_error = true;
        setErrorString("Invalid HMAC size.");
        return false;
    }

    QByteArray blockSizeBytes = m_baseDevice->read(4);
    if (blockSizeBytes.size() != 4) {
        m_error = true;
        setErrorString("Invalid block size size.");
        return false;
    }
    auto blockSize = Endian::bytesToSizedInt<qint32>(blockSizeBytes, ByteOrder);
    if (blockSize < 0) {
        m_error = true;
        setErrorString("Invalid block size.");
        return false;
    }

    m_buffer = m_baseDevice->read(blockSize);
    if (m_buffer.size() != blockSize) {
        m_error = true;
        setErrorString("Block too short.");
        return false;
    }

    CryptoHash hasher(CryptoHash::Sha256, true);
    hasher.setKey(getCurrentHmacKey());
    hasher.addData(Endian::sizedIntToBytes<quint64>(m_blockIndex, ByteOrder));
    hasher.addData(blockSizeBytes);
    hasher.addData(m_buffer);

    if (hmac != hasher.result()) {
        m_error = true;
        setErrorString("Mismatch between hash and data.");
        return false;
    }

    m_bufferPos = 0;
    ++m_blockIndex;

    if (blockSize == 0) {
        m_eof = true;
        return false;
    }

    return true;
}

qint64 HmacBlockStream::writeData(const char* data, qint64 maxSize)
{
    Q_ASSERT(maxSize >= 0);

    if (m_error) {
        return 0;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        qint64 bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(m_blockSize - m_buffer.size()));

        m_buffer.append(data + offset, static_cast<int>(bytesToCopy));

        offset += bytesToCopy;
        bytesRemaining -= bytesToCopy;

        if (m_buffer.size() == m_blockSize && !writeHashedBlock()) {
            if (m_error) {
                return -1;
            }
            return maxSize - bytesRemaining;
        }
    }

    return maxSize;
}

bool HmacBlockStream::writeHashedBlock()
{
    CryptoHash hasher(CryptoHash::Sha256, true);
    hasher.setKey(getCurrentHmacKey());
    hasher.addData(Endian::sizedIntToBytes<quint64>(m_blockIndex, ByteOrder));
    hasher.addData(Endian::sizedIntToBytes<qint32>(m_buffer.size(), ByteOrder));
    hasher.addData(m_buffer);
    QByteArray hash = hasher.result();

    if (m_baseDevice->write(hash) != hash.size()) {
        m_error = true;
        setErrorString(m_baseDevice->errorString());
        return false;
    }

    if (!Endian::writeSizedInt<qint32>(m_buffer.size(), m_baseDevice, ByteOrder)) {
        m_error = true;
        setErrorString(m_baseDevice->errorString());
        return false;
    }

    if (!m_buffer.isEmpty()) {
        if (m_baseDevice->write(m_buffer) != m_buffer.size()) {
            m_error = true;
            setErrorString(m_baseDevice->errorString());
            return false;
        }

        m_buffer.clear();
    }
    ++m_blockIndex;
    return true;
}

QByteArray HmacBlockStream::getCurrentHmacKey() const
{
    return getHmacKey(m_blockIndex, m_key);
}

QByteArray HmacBlockStream::getHmacKey(quint64 blockIndex, QByteArray key)
{
    Q_ASSERT(key.size() == 64);
    QByteArray indexBytes = Endian::sizedIntToBytes<quint64>(blockIndex, ByteOrder);
    CryptoHash hasher(CryptoHash::Sha512);
    hasher.addData(indexBytes);
    hasher.addData(key);
    return hasher.result();
}

bool HmacBlockStream::atEnd() const
{
    return m_eof;
}
