/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "OpData01.h"

#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"

#include <QDataStream>
#include <limits>

OpData01::OpData01(QObject* parent)
    : QObject(parent)
{
}

OpData01::~OpData01() = default;

bool OpData01::decodeBase64(QString const& b64String, const QByteArray& key, const QByteArray& hmacKey)
{
    const QByteArray b64Bytes = QByteArray::fromBase64(b64String.toUtf8());
    return decode(b64Bytes, key, hmacKey);
}

bool OpData01::decode(const QByteArray& data, const QByteArray& key, const QByteArray& hmacKey)
{
    /*!
     * The first 8 bytes of the data are the string “opdata01”.
     */
    const QByteArray header("opdata01");
    if (!data.startsWith(header)) {
        m_errorStr = tr("Invalid OpData01, does not contain header");
        return false;
    }

    constexpr auto lenBytes = static_cast<qsizetype>(sizeof(quint64));
    const qsizetype blockSize = SymmetricCipher::blockSize(SymmetricCipher::Aes256_CBC);
    const qsizetype prefixLen = header.size() + lenBytes + SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_CBC);
    constexpr qsizetype hmacLen = 256 / 8;
    if (data.size() < prefixLen + blockSize + hmacLen) {
        m_errorStr = tr("Invalid OpData01 data length");
        return false;
    }
    const qsizetype ciphertextLen = data.size() - prefixLen - hmacLen;
    if (ciphertextLen % blockSize != 0 || ciphertextLen > std::numeric_limits<int>::max()) {
        m_errorStr = tr("Invalid OpData01 ciphertext length");
        return false;
    }

    QDataStream in(data);
    in.setByteOrder(QDataStream::LittleEndian);
    in.skipRawData(header.size());

    /*!
     * The next 8 bytes contain the length in bytes of the plaintext as a little endian unsigned 64 bit integer.
     */
    quint64 len;
    in >> len;

    /*!
     * The plaintext is padded using the following scheme.
     *
     * If the size of the plaintext is an even multiple of the block size then 1 block of random data is prepended
     * to the plaintext. Otherwise, between 1 and 15 (inclusive) bytes of random data are prepended to the plaintext
     * to achieve an even multiple of blocks.
     */
    if (len >= static_cast<quint64>(ciphertextLen)
        || static_cast<quint64>(ciphertextLen) - len > static_cast<quint64>(blockSize)) {
        m_errorStr = tr("Plain text length out of range: %1").arg(len);
        return false;
    }
    const qsizetype randomBytes = ciphertextLen - static_cast<qsizetype>(len);

    /*!
     * The HMAC-SHA256 is computed over the entirety of the opdata including header, length, IV and ciphertext
     * using a 256-bit MAC key. The 256-bit MAC is not truncated. It is appended to the ciphertext.
     */
    const QByteArray hmacData = QByteArray::fromRawData(data.constData(), data.size() - hmacLen);
    const QByteArray hmacSig = data.right(hmacLen);
    const QByteArray hmacComputed = CryptoHash::hmac(hmacData, hmacKey, CryptoHash::Algorithm::Sha256);
    if (hmacComputed != hmacSig) {
        m_errorStr = tr("Malformed OpData01 due to a failed HMAC");
        return false;
    }

    const QByteArray iv = data.mid(header.size() + lenBytes, 16);
    SymmetricCipher cipher;
    if (!cipher.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Decrypt, key, iv)) {
        m_errorStr = tr("Unable to init cipher for opdata01: %1").arg(cipher.errorString());
        return false;
    }

    QByteArray qbaCT = data.mid(prefixLen, ciphertextLen);
    if (!cipher.process(qbaCT)) {
        m_errorStr = tr("Unable to process clearText in place");
        return false;
    }

    // Remove random bytes
    m_clearText = qbaCT.mid(randomBytes);
    return true;
}

QByteArray OpData01::getClearText()
{
    return m_clearText;
}

QString OpData01::errorString()
{
    return m_errorStr;
}
