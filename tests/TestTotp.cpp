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

#include <QDateTime>
#include <QTest>
#include <QTextCodec>
#include <QTime>
#include <QtEndian>

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
    QCOMPARE(QTotp::parseOtpString(secret, digits, step), QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    QCOMPARE(digits, quint8(6));
    QCOMPARE(step, quint8(30));

    digits = QTotp::defaultDigits;
    step = QTotp::defaultStep;
    secret = "key=HXDMVJECJJWSRBY%3d&step=25&size=8";
    QCOMPARE(QTotp::parseOtpString(secret, digits, step), QString("HXDMVJECJJWSRBY="));
    QCOMPARE(digits, quint8(8));
    QCOMPARE(step, quint8(25));

    digits = 0;
    step = 0;
    secret = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";
    QCOMPARE(QTotp::parseOtpString(secret, digits, step), QString("gezdgnbvgy3tqojqgezdgnbvgy3tqojq"));
    QCOMPARE(digits, quint8(6));
    QCOMPARE(step, quint8(30));
}

void TestTotp::testTotpCode()
{
    // Test vectors from RFC 6238
    // https://tools.ietf.org/html/rfc6238#appendix-B

    QByteArray seed = QString("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ").toLatin1();

    quint64 time = 1234567890;
    QString output = QTotp::generateTotp(seed, time, 6, 30);
    QCOMPARE(output, QString("005924"));

    time = 1111111109;
    output = QTotp::generateTotp(seed, time, 6, 30);
    QCOMPARE(output, QString("081804"));

    time = 1111111111;
    output = QTotp::generateTotp(seed, time, 8, 30);
    QCOMPARE(output, QString("14050471"));

    time = 2000000000;
    output = QTotp::generateTotp(seed, time, 8, 30);
    QCOMPARE(output, QString("69279037"));
}
