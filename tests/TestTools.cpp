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

#include "TestTools.h"

#include <QLocale>
#include <QTest>

QTEST_GUILESS_MAIN(TestTools)

namespace
{
    QString createDecimal(QString wholes, QString fractions, QString unit)
    {
        return wholes + QLocale().decimalPoint() + fractions + " " + unit;
    }
}

void TestTools::testHumanReadableFileSize()
{
    constexpr auto kibibyte = 1024u;
    using namespace Tools;

    QCOMPARE(createDecimal("1", "00", "B"), humanReadableFileSize(1));
    QCOMPARE(createDecimal("1", "00", "KiB"), humanReadableFileSize(kibibyte));
    QCOMPARE(createDecimal("1", "00", "MiB"), humanReadableFileSize(kibibyte * kibibyte));
    QCOMPARE(createDecimal("1", "00", "GiB"), humanReadableFileSize(kibibyte * kibibyte * kibibyte));

    QCOMPARE(QString("100 B"), humanReadableFileSize(100, 0));
    QCOMPARE(createDecimal("1", "10", "KiB"), humanReadableFileSize(kibibyte + 100));
    QCOMPARE(createDecimal("1", "001", "KiB"), humanReadableFileSize(kibibyte + 1, 3));
    QCOMPARE(createDecimal("15", "00", "KiB"), humanReadableFileSize(kibibyte * 15));
}

void TestTools::testIsHex()
{
    QVERIFY(Tools::isHex("0123456789abcdefABCDEF"));
    QVERIFY(not Tools::isHex(QByteArray("0xnothex")));
}

void TestTools::testIsBase64()
{
    QVERIFY(Tools::isBase64(QByteArray("1234")));
    QVERIFY(Tools::isBase64(QByteArray("123=")));
    QVERIFY(Tools::isBase64(QByteArray("12==")));
    QVERIFY(Tools::isBase64(QByteArray("abcd9876MN==")));
    QVERIFY(Tools::isBase64(QByteArray("abcd9876DEFGhijkMNO=")));
    QVERIFY(not Tools::isBase64(QByteArray("abcd123==")));
    QVERIFY(not Tools::isBase64(QByteArray("abc_")));
    QVERIFY(not Tools::isBase64(QByteArray("123")));
}
