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

const quint8 QTotp::defaultStep = 30;
const quint8 QTotp::defaultDigits = 6;

QTotp::QTotp()
{
}

QString QTotp::parseOtpString(QString key, quint8& digits, quint8& step)
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

QString QTotp::generateTotp(const QByteArray key,
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

    quint32 digitsPower = pow(10, numDigits);

    quint64 password = binary % digitsPower;
    return QString("%1").arg(password, numDigits, 10, QChar('0'));
}

// See: https://github.com/google/google-authenticator/wiki/Key-Uri-Format
QUrl QTotp::generateOtpString(const QString& secret,
                              const QString& type,
                              const QString& issuer,
                              const QString& username,
                              const QString& algorithm,
                              const quint8& digits,
                              const quint8& step)
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
