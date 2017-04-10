#include "totp.h"
#include <QtEndian>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>


QTotp::QTotp()
{
}

/** 
 * Base32 implementation
 *
 * Copyright 2010 Google Inc.
 * Author: Markus Gutschke
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

int QTotp::base32_decode(const quint8 *encoded, quint8 *result, int bufSize)
{
    int buffer = 0;
    int bitsLeft = 0;
    int count = 0;

    for (const quint8 *ptr = encoded; count < bufSize && *ptr; ++ptr) {
        quint8 ch = *ptr;

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
            continue;
        }

        buffer <<= 5;

        // Deal with commonly mistyped characters
        if (ch == '0') {
            ch = 'O';
        } else if (ch == '1') {
            ch = 'L';
        } else if (ch == '8') {
            ch = 'B';
        }

        // Look up one base32 digit
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            ch = (ch & 0x1F) - 1;
        } else if (ch >= '2' && ch <= '7') {
            ch -= '2' - 26;
        } else {
            return -1;
        }

        buffer |= ch;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result[count++] = buffer >> (bitsLeft - 8);
            bitsLeft -= 8;
        }
    }

    if (count < bufSize) {
        result[count] = '\000';
    }

    return count;
}


QString QTotp::generateTotp(const QByteArray key)
{
    quint64 time = QDateTime::currentDateTime().toTime_t();
    quint64 current = qToBigEndian(time / 30);
    const unsigned numDigits = 6;

    int secretLen = (key.length() + 7) / 8 * 5;
    quint8 secret[secretLen];
    int res = QTotp::base32_decode(reinterpret_cast<const quint8 *>(key.constData()), secret, secretLen);

    QMessageAuthenticationCode code(QCryptographicHash::Sha1);
    code.setKey(QByteArray(reinterpret_cast<const char *>(secret), res));
    code.addData(QByteArray(reinterpret_cast<char*>(&current), sizeof(current)));
    QByteArray hmac = code.result();

    int offset = (hmac[hmac.length() - 1] & 0xf);
    int binary =
            ((hmac[offset] & 0x7f) << 24)
            | ((hmac[offset + 1] & 0xff) << 16)
            | ((hmac[offset + 2] & 0xff) << 8)
            | (hmac[offset + 3] & 0xff);

    int password = binary % 1000000;
    return QString("%1").arg(password, numDigits, 10, QChar('0'));
}
