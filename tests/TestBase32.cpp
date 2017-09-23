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

#include "TestBase32.h"
#include "tools/base32.h"
#include <QTest>

QTEST_GUILESS_MAIN(TestBase32)

void TestBase32::testDecode()
{
    // 3 quantums, all upper case + padding
    QByteArray encodedData = "JBSWY3DPEB3W64TMMQXC4LQ=";
    auto data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("Hello world..."));

    // 4 quantums, all upper case
    encodedData = "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("12345678901234567890"));

    // 4 quantums, all lower case
    encodedData = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("12345678901234567890"));

    // 4 quantums, mixed upper and lower case
    encodedData = "Gezdgnbvgy3tQojqgezdGnbvgy3tQojQ";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("12345678901234567890"));

    // 1 pad characters
    encodedData = "ORSXG5A=";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("test"));

    // 3 pad characters
    encodedData = "L5PV6===";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("___"));

    // 4 pad characters
    encodedData = "MZXW6IDCMFZA====";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("foo bar"));

    // six pad characters
    encodedData = "MZXW6YTBOI======";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("foobar"));

    encodedData = "IA======";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("@"));

    // error: illegal character
    encodedData = "1MZXW6YTBOI=====";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("ERROR"));

    // error: missing pad character
    encodedData = "MZXW6YTBOI=====";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("ERROR"));

    // RFC 4648 test vectors
    encodedData = "";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString(""));

    encodedData = "MY======";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("f"));

    encodedData = "MZXQ====";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("fo"));

    encodedData = "MZXW6===";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("foo"));

    encodedData = "MZXW6YQ=";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("foob"));

    encodedData = "MZXW6YTB";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("fooba"));

    encodedData = "MZXW6YTBOI======";
    data = Base32::decode(encodedData);
    QCOMPARE(QString::fromLatin1(data.valueOr("ERROR")), QString("foobar"));
}

void TestBase32::testEncode()
{
    QByteArray data = "Hello world...";
    QByteArray encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("JBSWY3DPEB3W64TMMQXC4LQ="));

    data = "12345678901234567890";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ")); 

    data = "012345678901234567890";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("GAYTEMZUGU3DOOBZGAYTEMZUGU3DOOBZGA======"));

    data = "test";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("ORSXG5A=")); 

    data = "___";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("L5PV6===")); 

    data = "foo bar";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXW6IDCMFZA====")); 

    data = "@";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("IA======")); 

    // RFC 4648 test vectors
    data = "";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("")); 

    data = "f";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MY======")); 

    data = "fo";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXQ====")); 

    data = "foo";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXW6==="));
 
    data = "foob";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXW6YQ=")); 

    data = "fooba";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXW6YTB")); 

    data = "foobar";
    encodedData = Base32::encode(data);
    QCOMPARE(encodedData, QByteArray("MZXW6YTBOI======")); 
}
