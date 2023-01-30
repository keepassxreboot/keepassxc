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

    QDataStream in(data);
    in.setByteOrder(QDataStream::LittleEndian);
    in.skipRawData(header.size());

    /*!
     * The next 8 bytes contain the length in bytes of the plaintext as a little endian unsigned 64 bit integer.
     */
    qlonglong len;
    in >> len;

    /*!
     * The next 16 bytes are the randomly chosen initialization vector.
     */
    QByteArray iv(16, '\0');
    int read = in.readRawData(iv.data(), 16);
    if (read != 16) {
        m_errorStr = tr("Unable to read all IV bytes, wanted 16 but got %1").arg(iv.size());
        return false;
    }

    SymmetricCipher cipher;
    if (!cipher.init(SymmetricCipher::Aes256_CBC, SymmetricCipher::Decrypt, key, iv)) {
        m_errorStr = tr("Unable to init cipher for opdata01: %1").arg(cipher.errorString());
        return false;
    }

    /*!
     * The plaintext is padded using the following scheme.
     *
     * If the size of the plaintext is an even multiple of the block size then 1 block of random data is prepended
     * to the plaintext. Otherwise, between 1 and 15 (inclusive) bytes of random data are prepended to the plaintext
     * to achieve an even multiple of blocks.
     */
    const int blockSize = cipher.blockSize(cipher.mode());
    int randomBytes = blockSize - (len % blockSize);
    if (randomBytes == 0) {
        // add random block
        randomBytes = blockSize;
    }
    qlonglong clear_len = len + randomBytes;
    QByteArray qbaCT(clear_len, '\0');
    in.readRawData(qbaCT.data(), clear_len);

    /*!
     * The HMAC-SHA256 is computed over the entirety of the opdata including header, length, IV and ciphertext
     * using a 256-bit MAC key. The 256-bit MAC is not truncated. It is appended to the ciphertext.
     */
    const int hmacLen = 256 / 8;
    QByteArray hmacSig(hmacLen, '\0'); // 256 / 8, '\0');
    in.readRawData(hmacSig.data(), hmacLen);
    if (hmacSig.size() != hmacLen) {
        m_errorStr = tr("Unable to read all HMAC signature bytes");
        return false;
    }

    const QByteArray hmacData = data.mid(0, data.size() - hmacSig.size());
    const QByteArray actualHmac = CryptoHash::hmac(hmacData, hmacKey, CryptoHash::Algorithm::Sha256);
    if (actualHmac != hmacSig) {
        m_errorStr = tr("Malformed OpData01 due to a failed HMAC");
        return false;
    }

    if (!cipher.process(qbaCT)) {
        m_errorStr = tr("Unable to process clearText in place");
        return false;
    }

    // Remove random bytes
    const QByteArray& clearText = qbaCT.mid(randomBytes);
    if (clearText.size() != len) {
        m_errorStr = tr("Expected %1 bytes of clear-text, found %2").arg(len, clearText.size());
        return false;
    }
    m_clearText = clearText;
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
