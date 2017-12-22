/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
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

#include "totp.h"
#include "core/Base32.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageAuthenticationCode>
#include <QRegExp>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QtEndian>
#include <cmath>

const quint8 Totp::defaultStep = 30;
const quint8 Totp::defaultDigits = 6;

/**
 * Custom encoder types. Each should be unique and >= 128 and < 255
 * Values have no meaning outside of keepassxc
 */
/**
 * Encoder for Steam Guard TOTP
 */
const quint8 Totp::ENCODER_STEAM = 254;

const Totp::Encoder Totp::defaultEncoder = { "", "", "0123456789", 0, 0, false };
const QMap<quint8, Totp::Encoder> Totp::encoders{
    { Totp::ENCODER_STEAM, { "steam", "S", "23456789BCDFGHJKMNPQRTVWXY", 5, 30, true } },
};

/**
 * These map the second field of the "TOTP Settings" field to our internal encoder number
 * that overloads the digits field. Make sure that the key matches the shortName value
 * in the corresponding Encoder
 * NOTE: when updating this map, a corresponding edit to the settings regex must be made
 *       in Entry::totpSeed()
 */
const QMap<QString, quint8> Totp::shortNameToEncoder{
    { "S", Totp::ENCODER_STEAM },
};
/**
 * These map the "encoder=" URL parameter of the "otp" field to our internal encoder number
 * that overloads the digits field. Make sure that the key matches the name value
 * in the corresponding Encoder
 */
const QMap<QString, quint8> Totp::nameToEncoder{
    { "steam", Totp::ENCODER_STEAM },
};

Totp::Totp()
{
}

QString Totp::parseOtpString(QString key, quint8& digits, quint8& step)
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
        QString encName = query.queryItemValue("encoder");
        if (!encName.isEmpty() && nameToEncoder.contains(encName)) {
            digits = nameToEncoder[encName];
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

QString Totp::generateTotp(const QByteArray key,
                            quint64 time,
                            const quint8 numDigits = defaultDigits,
                            const quint8 step = defaultStep)
{
    quint64 current = qToBigEndian(time / step);

    QVariant secret = Base32::decode(Base32::sanitizeInput(key));
    if (secret.isNull()) {
        return "Invalid TOTP secret key";
    }

    QMessageAuthenticationCode code(QCryptographicHash::Sha1);
    code.setKey(secret.toByteArray());
    code.addData(QByteArray(reinterpret_cast<char*>(&current), sizeof(current)));
    QByteArray hmac = code.result();

    int offset = (hmac[hmac.length() - 1] & 0xf);

    // clang-format off
    int binary =
            ((hmac[offset] & 0x7f) << 24)
            | ((hmac[offset + 1] & 0xff) << 16)
            | ((hmac[offset + 2] & 0xff) << 8)
            | (hmac[offset + 3] & 0xff);
    // clang-format on

    const Encoder& encoder = encoders.value(numDigits, defaultEncoder);
    // if encoder.digits is 0, we need to use the passed-in number of digits (default encoder)
    quint8 digits = encoder.digits == 0 ? numDigits : encoder.digits;
    int direction = -1;
    int startpos = digits - 1;
    if (encoder.reverse) {
        direction = 1;
        startpos = 0;
    }
    quint32 digitsPower = pow(encoder.alphabet.size(), digits);

    quint64 password = binary % digitsPower;
    QString retval(int(digits), encoder.alphabet[0]);
    for (quint8 pos = startpos; password > 0; pos += direction) {
        retval[pos] = encoder.alphabet[int(password % encoder.alphabet.size())];
        password /= encoder.alphabet.size();
    }
    return retval;
}

// See: https://github.com/google/google-authenticator/wiki/Key-Uri-Format
QUrl Totp::generateOtpString(const QString& secret,
                              const QString& type,
                              const QString& issuer,
                              const QString& username,
                              const QString& algorithm,
                              quint8 digits,
                              quint8 step)
{
    QUrl keyUri;
    keyUri.setScheme("otpauth");
    keyUri.setHost(type);
    keyUri.setPath(QString("/%1:%2").arg(issuer).arg(username));
    QUrlQuery parameters;
    parameters.addQueryItem("secret", secret);
    parameters.addQueryItem("issuer", issuer);
    parameters.addQueryItem("algorithm", algorithm);
    parameters.addQueryItem("digits", QString::number(digits));
    parameters.addQueryItem("period", QString::number(step));
    keyUri.setQuery(parameters);

    return keyUri;
}
