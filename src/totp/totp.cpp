/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
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

#include "totp.h"
#include <cmath>
#include <QtEndian>
#include <QRegExp>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QUrlQuery>


const quint8 QTotp::defaultStep = 30;
const quint8 QTotp::defaultDigits = 6;

QTotp::QTotp()
{
}

QString QTotp::parseOtpString(QString key, quint8 &digits, quint8 &step)
{
    QUrl url(key);

    QString seed;
    uint q_digits, q_step;

    // Default OTP url format
    if (url.isValid() && url.scheme() == "otpauth") {
        QUrlQuery query(url);

        seed = query.queryItemValue("secret");

        q_digits = query.queryItemValue("digits").toUInt();
        if (q_digits == 6 || q_digits == 8) {
            digits = q_digits;
        }

        q_step = query.queryItemValue("period").toUInt();
        if (q_step > 0 && q_step <= 60) {
            step = q_step;
        }


    } else {
        // Compatibility with "KeeOtp" plugin string format
        QRegExp rx("key=(.+)", Qt::CaseInsensitive, QRegExp::RegExp);

        if (rx.exactMatch(key)) {
            QUrlQuery query(key);

            seed = query.queryItemValue("key");
            q_digits = query.queryItemValue("size").toUInt();
            if (q_digits == 6 || q_digits == 8) {
                digits = q_digits;
            }

            q_step = query.queryItemValue("step").toUInt();
            if (q_step > 0 && q_step <= 60) {
                step = q_step;
            }

        } else {
            seed = key;
        }
    }

    if (digits == 0) {
        digits = defaultDigits;
    }

    if (step == 0) {
        step = defaultStep;
    }

    return seed;
}


QByteArray QTotp::base32_decode(const QByteArray encoded)
{
    // Base32 implementation
    // Copyright 2010 Google Inc.
    // Author: Markus Gutschke
    // Licensed under the Apache License, Version 2.0

    QByteArray result;

    int buffer = 0;
    int bitsLeft = 0;

    for (char ch : encoded) {
        if (ch == 0 || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-' || ch == '=') {
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
            return QByteArray();
        }

        buffer |= ch;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result.append(static_cast<char> (buffer >> (bitsLeft - 8)));
            bitsLeft -= 8;
        }
    }

    return result;
}


QString QTotp::generateTotp(const QByteArray key, quint64 time, const quint8 numDigits = defaultDigits, const quint8 step = defaultStep)
{
    quint64 current = qToBigEndian(time / step);

    QByteArray secret = QTotp::base32_decode(key);
    if (secret.isEmpty()) {
        return "Invalid TOTP secret key";
    }

    QMessageAuthenticationCode code(QCryptographicHash::Sha1);
    code.setKey(secret);
    code.addData(QByteArray(reinterpret_cast<char*>(&current), sizeof(current)));
    QByteArray hmac = code.result();

    int offset = (hmac[hmac.length() - 1] & 0xf);
    int binary =
            ((hmac[offset] & 0x7f) << 24)
            | ((hmac[offset + 1] & 0xff) << 16)
            | ((hmac[offset + 2] & 0xff) << 8)
            | (hmac[offset + 3] & 0xff);

    quint32 digitsPower = pow(10, numDigits);

    quint64 password = binary % digitsPower;
    return QString("%1").arg(password, numDigits, 10, QChar('0'));
}
