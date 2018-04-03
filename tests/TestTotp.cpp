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

#include "TestTotp.h"
#include "TestGlobal.h"

#include "crypto/Crypto.h"
#include "totp/totp.h"

QTEST_GUILESS_MAIN(TestTotp)

void TestTotp::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestTotp::testParseSecret()
{
    quint8 digits = 0;
    quint8 step = 0;
    QString secret = "otpauth://totp/"
                     "ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
                     "SHA1&digits=6&period=30";
    QCOMPARE(Totp::parseOtpString(secret, digits, step), QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    QCOMPARE(digits, quint8(6));
    QCOMPARE(step, quint8(30));

    digits = Totp::defaultDigits;
    step = Totp::defaultStep;
    secret = "key=HXDMVJECJJWSRBY%3d&step=25&size=8";
    QCOMPARE(Totp::parseOtpString(secret, digits, step), QString("HXDMVJECJJWSRBY="));
    QCOMPARE(digits, quint8(8));
    QCOMPARE(step, quint8(25));

    digits = 0;
    step = 0;
    secret = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";
    QCOMPARE(Totp::parseOtpString(secret, digits, step), QString("gezdgnbvgy3tqojqgezdgnbvgy3tqojq"));
    QCOMPARE(digits, quint8(6));
    QCOMPARE(step, quint8(30));
}

void TestTotp::testTotpCode()
{
    // Test vectors from RFC 6238
    // https://tools.ietf.org/html/rfc6238#appendix-B

    QByteArray seed = QString("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ").toLatin1();

    quint64 time = 1234567890;
    QString output = Totp::generateTotp(seed, time, 6, 30);
    QCOMPARE(output, QString("005924"));

    time = 1111111109;
    output = Totp::generateTotp(seed, time, 6, 30);
    QCOMPARE(output, QString("081804"));

    time = 1111111111;
    output = Totp::generateTotp(seed, time, 8, 30);
    QCOMPARE(output, QString("14050471"));

    time = 2000000000;
    output = Totp::generateTotp(seed, time, 8, 30);
    QCOMPARE(output, QString("69279037"));
}

void TestTotp::testEncoderData()
{
    for (quint8 key : Totp::encoders.keys()) {
        const Totp::Encoder& enc = Totp::encoders.value(key);
        QVERIFY2(
            enc.digits != 0,
            qPrintable(
                QString("Custom encoders cannot have zero-value for digits field: %1(%2)").arg(enc.name).arg(key)));
        QVERIFY2(!enc.name.isEmpty(),
                 qPrintable(QString("Custom encoders must have a name: %1(%2)").arg(enc.name).arg(key)));
        QVERIFY2(!enc.shortName.isEmpty(),
                 qPrintable(QString("Custom encoders must have a shortName: %1(%2)").arg(enc.name).arg(key)));
        QVERIFY2(Totp::shortNameToEncoder.contains(enc.shortName),
                 qPrintable(QString("No shortNameToEncoder entry found for custom encoder: %1(%2) %3")
                                .arg(enc.name)
                                .arg(key)
                                .arg(enc.shortName)));
        QVERIFY2(Totp::shortNameToEncoder[enc.shortName] == key,
                 qPrintable(QString("shortNameToEncoder doesn't reference this custome encoder: %1(%2) %3")
                                .arg(enc.name)
                                .arg(key)
                                .arg(enc.shortName)));
        QVERIFY2(Totp::nameToEncoder.contains(enc.name),
                 qPrintable(QString("No nameToEncoder entry found for custom encoder: %1(%2) %3")
                                .arg(enc.name)
                                .arg(key)
                                .arg(enc.shortName)));
        QVERIFY2(Totp::nameToEncoder[enc.name] == key,
                 qPrintable(QString("nameToEncoder doesn't reference this custome encoder: %1(%2) %3")
                                .arg(enc.name)
                                .arg(key)
                                .arg(enc.shortName)));
    }

    for (const QString& key : Totp::nameToEncoder.keys()) {
        quint8 value = Totp::nameToEncoder.value(key);
        QVERIFY2(Totp::encoders.contains(value),
                 qPrintable(QString("No custom encoder found for encoder named %1(%2)").arg(value).arg(key)));
        QVERIFY2(Totp::encoders[value].name == key,
                 qPrintable(
                     QString("nameToEncoder doesn't reference the right custom encoder: %1(%2)").arg(value).arg(key)));
    }

    for (const QString& key : Totp::shortNameToEncoder.keys()) {
        quint8 value = Totp::shortNameToEncoder.value(key);
        QVERIFY2(Totp::encoders.contains(value),
                 qPrintable(QString("No custom encoder found for short-name encoder %1(%2)").arg(value).arg(key)));
        QVERIFY2(
            Totp::encoders[value].shortName == key,
            qPrintable(
                QString("shortNameToEncoder doesn't reference the right custom encoder: %1(%2)").arg(value).arg(key)));
    }
}

void TestTotp::testSteamTotp()
{
    quint8 digits = 0;
    quint8 step = 0;
    QString secret = "otpauth://totp/"
                     "test:test@example.com?secret=63BEDWCQZKTQWPESARIERL5DTTQFCJTK&issuer=Valve&algorithm="
                     "SHA1&digits=5&period=30&encoder=steam";
    QCOMPARE(Totp::parseOtpString(secret, digits, step), QString("63BEDWCQZKTQWPESARIERL5DTTQFCJTK"));
    QCOMPARE(digits, quint8(Totp::ENCODER_STEAM));
    QCOMPARE(step, quint8(30));

    QByteArray seed = QString("63BEDWCQZKTQWPESARIERL5DTTQFCJTK").toLatin1();

    // These time/value pairs were created by running the Steam Guard function of the
    // Steam mobile app with a throw-away steam account. The above secret was extracted
    // from the Steam app's data for use in testing here.
    quint64 time = 1511200518;
    QCOMPARE(Totp::generateTotp(seed, time, Totp::ENCODER_STEAM, 30), QString("FR8RV"));
    time = 1511200714;
    QCOMPARE(Totp::generateTotp(seed, time, Totp::ENCODER_STEAM, 30), QString("9P3VP"));
}

void TestTotp::testEntryHistory()
{
    Entry entry;
    quint8 step = 16;
    quint8 digits = 6;
    QCOMPARE(entry.historyItems().size(), 0);
    entry.setTotp("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ", step, digits);
    QCOMPARE(entry.historyItems().size(), 1);
    entry.setTotp("foo", step, digits);
    QCOMPARE(entry.historyItems().size(), 2);
}
