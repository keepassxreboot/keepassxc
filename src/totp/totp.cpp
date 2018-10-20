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
#include "core/Clock.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QtEndian>
#include <cmath>

static QList<Totp::Encoder> encoders {
    {"", "", "0123456789", Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP, false},
    {"steam", Totp::STEAM_SHORTNAME, "23456789BCDFGHJKMNPQRTVWXY", Totp::STEAM_DIGITS, Totp::DEFAULT_STEP, true},
};

QSharedPointer<Totp::Settings> Totp::parseSettings(const QString& rawSettings, const QString& key)
{
    // Create default settings
    auto settings = createSettings(key, DEFAULT_DIGITS, DEFAULT_STEP);

    QUrl url(rawSettings);
    if (url.isValid() && url.scheme() == "otpauth") {
        // Default OTP url format
        QUrlQuery query(url);
        settings->otpUrl = true;
        settings->key = query.queryItemValue("secret");
        settings->digits = query.queryItemValue("digits").toUInt();
        settings->step = query.queryItemValue("period").toUInt();
        if (query.hasQueryItem("encoder")) {
            settings->encoder = getEncoderByName(query.queryItemValue("encoder"));
        }
    } else {
        QUrlQuery query(rawSettings);
        if (query.hasQueryItem("key")) {
            // Compatibility with "KeeOtp" plugin
            // if settings are changed, will convert to semi-colon format
            settings->key = query.queryItemValue("key");
            settings->digits = query.queryItemValue("size").toUInt();
            settings->step = query.queryItemValue("step").toUInt();
        } else {
            // Parse semi-colon separated values ([step];[digits|S])
            auto vars = rawSettings.split(";");
            if (vars.size() >= 2) {
                if (vars[1] == STEAM_SHORTNAME) {
                    // Explicit steam encoder
                    settings->encoder = steamEncoder();
                } else {
                    // Extract step and digits
                    settings->step = vars[0].toUInt();
                    settings->digits = vars[1].toUInt();
                }
            }
        }
    }

    // Bound digits and step
    settings->digits = qMax(1u, settings->digits);
    settings->step = qBound(1u, settings->step, 60u);

    // Detect custom settings, used by setup GUI
    if (settings->encoder.shortName.isEmpty()
        && (settings->digits != DEFAULT_DIGITS || settings->step != DEFAULT_STEP)) {
        settings->custom = true;
    }

    return settings;
}

QSharedPointer<Totp::Settings> Totp::createSettings(const QString& key, const uint digits, const uint step,
                                                    const QString& encoderShortName)
{
    bool isCustom = digits != DEFAULT_DIGITS || step != DEFAULT_STEP;
    return QSharedPointer<Totp::Settings>(new Totp::Settings {
        getEncoderByShortName(encoderShortName), key, false, isCustom, digits, step
    });
}

QString Totp::writeSettings(const QSharedPointer<Totp::Settings> settings, const QString& title, const QString& username, bool forceOtp)
{
    if (settings.isNull()) {
        return {};
    }

    // OTP Url output
    if (settings->otpUrl || forceOtp) {
        auto urlstring = QString("otpauth://totp/%1:%2?secret=%3&period=%4&digits=%5&issuer=%1")
                .arg(title.isEmpty() ? "KeePassXC" : QString(QUrl::toPercentEncoding(title)))
                .arg(username.isEmpty() ? "none" : QString(QUrl::toPercentEncoding(username)))
                .arg(QString(Base32::sanitizeInput(settings->key.toLatin1())))
                .arg(settings->step)
                .arg(settings->digits);

        if (!settings->encoder.name.isEmpty()) {
            urlstring.append("&encoder=").append(settings->encoder.name);
        }
        return urlstring;
    }

    // Semicolon output [step];[encoder]
    if (!settings->encoder.shortName.isEmpty()) {
        return QString("%1;%2").arg(settings->step).arg(settings->encoder.shortName);
    }

    // Semicolon output [step];[digits]
    return QString("%1;%2").arg(settings->step).arg(settings->digits);
}

QString Totp::generateTotp(const QSharedPointer<Totp::Settings> settings, const quint64 time)
{
    Q_ASSERT(!settings.isNull());
    if (settings.isNull()) {
        return QObject::tr("Invalid Settings", "TOTP");
    }

    const Encoder& encoder = settings->encoder;
    uint step = settings->custom ? settings->step : encoder.step;
    uint digits = settings->custom ? settings->digits : encoder.digits;

    quint64 current;
    if (time == 0) {
        current = qToBigEndian(static_cast<quint64>(Clock::currentSecondsSinceEpoch()) / step);
    } else {
        current = qToBigEndian(time / step);
    }

    QVariant secret = Base32::decode(Base32::sanitizeInput(settings->key.toLatin1()));
    if (secret.isNull()) {
        return QObject::tr("Invalid Key", "TOTP");
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

Totp::Encoder& Totp::defaultEncoder()
{
    // The first encoder is always the default
    Q_ASSERT(!encoders.empty());
    return encoders[0];
}

Totp::Encoder& Totp::steamEncoder()
{
    return getEncoderByShortName("S");
}

Totp::Encoder& Totp::getEncoderByShortName(QString shortName)
{
    for (auto& encoder : encoders) {
        if (encoder.shortName == shortName) {
            return encoder;
        }
    }
    return defaultEncoder();
}

Totp::Encoder& Totp::getEncoderByName(QString name)
{
    for (auto& encoder : encoders) {
        if (encoder.name == name) {
            return encoder;
        }
    }
    return defaultEncoder();
}
