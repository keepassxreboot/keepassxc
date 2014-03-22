/* This file is part of QJson
 *
 * Copyright (C) 2013 Silvio Moioli <silvio@moioli.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software Foundation.
 * 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
 
#include <cmath>

#include <QtCore/QVariant>
#include <QtCore/QVariant>

#include <QtTest/QtTest>

#include "json_scanner.h"
#include "json_parser.hh"
#include "location.hh"

#define TOKEN(type) (int)yy::json_parser::token::type

class TestScanner: public QObject
{
  Q_OBJECT
  private slots:
    void scanClosedDevice();
    void scanTokens();
    void scanTokens_data();
    void scanSpecialNumbers();
    void scanSpecialNumbers_data();
};

Q_DECLARE_METATYPE(QVariant)
Q_DECLARE_METATYPE(QVariant::Type)

using namespace QJson;

void TestScanner::scanClosedDevice() {
  QBuffer buffer;
  int expectedResult = -1;
  
  JSonScanner scanner(&buffer);
  QVariant yylval;
  yy::location location;
  int result = scanner.yylex(&yylval, &location);
  QCOMPARE(result, expectedResult);
}

void TestScanner::scanTokens() {
  QFETCH(QByteArray, input);
  QFETCH(bool, allowSpecialNumbers);
  QFETCH(bool, skipFirstToken);
  QFETCH(int, expectedResult);
  QFETCH(QVariant, expectedYylval);
  QFETCH(int, expectedLocationBeginLine);
  QFETCH(int, expectedLocationBeginColumn);
  QFETCH(int, expectedLocationEndLine);
  QFETCH(int, expectedLocationEndColumn);
  
  QBuffer buffer;
  buffer.open(QBuffer::ReadWrite);
  buffer.write(input);
  buffer.seek(0);  
  JSonScanner scanner(&buffer);
  scanner.allowSpecialNumbers(allowSpecialNumbers);

  QVariant yylval;
  yy::position position(YY_NULL, 1, 0);
  yy::location location(position, position);
  int result = scanner.yylex(&yylval, &location);
  
  if (skipFirstToken) {
    result = scanner.yylex(&yylval, &location);
  }
  
  QCOMPARE(result, expectedResult);
  QCOMPARE(yylval, expectedYylval);
  QCOMPARE(location.begin.line, (uint)expectedLocationBeginLine);
  QCOMPARE(location.begin.column, (uint)expectedLocationBeginColumn);
  QCOMPARE(location.end.line, (uint)expectedLocationEndLine);
  QCOMPARE(location.end.column, (uint)expectedLocationEndColumn);
}

void TestScanner::scanTokens_data() {
  QTest::addColumn<QByteArray>("input");
  QTest::addColumn<bool>("allowSpecialNumbers");
  QTest::addColumn<bool>("skipFirstToken");
  QTest::addColumn<int>("expectedResult");
  QTest::addColumn<QVariant>("expectedYylval");
  QTest::addColumn<int>("expectedLocationBeginLine");
  QTest::addColumn<int>("expectedLocationBeginColumn");
  QTest::addColumn<int>("expectedLocationEndLine");
  QTest::addColumn<int>("expectedLocationEndColumn");
  
  QTest::newRow("empty string") << QByteArray("") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 1 << 0;
  
  QTest::newRow("carriage return") << QByteArray("\r") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 2 << 1;
  QTest::newRow("new line") << QByteArray("\n") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 2 << 1;
  QTest::newRow("formfeed") << QByteArray("\f") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("vertical tab") << QByteArray("\v") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("space") << QByteArray(" ") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("tab") << QByteArray("\t") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("all spaces") << QByteArray("\r\n\f\v \t") << true << false << TOKEN(END) << QVariant() << 1 << 0 << 3 << 5;
  
  QTest::newRow("true") << QByteArray("true") << true << false << TOKEN(TRUE_VAL) << QVariant(true) << 1 << 0 << 1 << 4;
  QTest::newRow("false") << QByteArray("false") << true << false << TOKEN(FALSE_VAL) << QVariant(false) << 1 << 0 << 1 << 5;
  QTest::newRow("null") << QByteArray("null") << true << false << TOKEN(NULL_VAL) << QVariant() << 1 << 0 << 1 << 4;
  
  QTest::newRow("alphabetic string") << QByteArray("\"abcde\"") << true << false << TOKEN(STRING) << QVariant(QLatin1String("abcde")) << 1 << 0 << 1 << 2;
  QTest::newRow("ecaped string") << QByteArray("\"abcde\\b\\f\\n\\r\\t\"") << true << false << TOKEN(STRING) << QVariant(QLatin1String("abcde\b\f\n\r\t")) << 1 << 0 << 1 << 2;
  QTest::newRow("invalid ecaped string") << QByteArray("\"\\x\"") << true << false << TOKEN(STRING) << QVariant(QLatin1String("x")) << 1 << 0 << 1 << 2;
  QTest::newRow("escaped unicode sequence") << QByteArray("\"\\u005A\"") << true << false << TOKEN(STRING) << QVariant(QLatin1String("Z"))  << 1 << 0 << 1 << 2;
  QTest::newRow("invalid unicode sequence") << QByteArray("\"\\u005Z\"") << true << false << TOKEN(INVALID) << QVariant(QLatin1String(""))  << 1 << 0 << 1 << 2;
  QTest::newRow("empty string") << QByteArray("\"") << true << false << TOKEN(END) << QVariant()  << 1 << 0 << 1 << 1;
  
  QTest::newRow("single digit") << QByteArray("0") << true << false << TOKEN(NUMBER) << QVariant(0u)  << 1 << 0 << 1 << 1;
  QTest::newRow("multiple digits") << QByteArray("123456789") << true << false << TOKEN(NUMBER) << QVariant(123456789u)  << 1 << 0 << 1 << 9;
  QTest::newRow("negative single digit") << QByteArray("-0") << true << false << TOKEN(NUMBER) << QVariant(0)  << 1 << 0 << 1 << 2;
  QTest::newRow("negative multiple digits") << QByteArray("-123456789") << true << false << TOKEN(NUMBER) << QVariant(-123456789)  << 1 << 0 << 1 << 10;
  QTest::newRow("fractional single digit") << QByteArray("0.1") << true << false << TOKEN(NUMBER) << QVariant(0.1)  << 1 << 0 << 1 << 3;
  QTest::newRow("fractional multiple digits") << QByteArray("123456789.12") << true << false << TOKEN(NUMBER) << QVariant(123456789.12)  << 1 << 0 << 1 << 12;
  QTest::newRow("fractional negative single digit") << QByteArray("-0.3") << true << false << TOKEN(NUMBER) << QVariant(-0.3)  << 1 << 0 << 1 << 4;
  QTest::newRow("fractional negative multiple digits") << QByteArray("-123456789.23") << true << false << TOKEN(NUMBER) << QVariant(-123456789.23)  << 1 << 0 << 1 << 13;
  QTest::newRow("exponential single digit") << QByteArray("10e2") << true << false << TOKEN(NUMBER) << QVariant(1000)  << 1 << 0 << 1 << 4;
  QTest::newRow("exponential multiple digits") << QByteArray("10e23") << true << false << TOKEN(NUMBER) << QVariant(10e23)  << 1 << 0 << 1 << 5;
  QTest::newRow("exponential zero") << QByteArray("0e23") << true << false << TOKEN(NUMBER) << QVariant(0)  << 1 << 0 << 1 << 4;
  QTest::newRow("exponential fractional") << QByteArray("0.12354e23") << true << false << TOKEN(NUMBER) << QVariant(0.12354e23)  << 1 << 0 << 1 << 10;
  QTest::newRow("exponential fractional multiple digits") << QByteArray("120.12354e23") << true << false << TOKEN(NUMBER) << QVariant(120.12354e23)  << 1 << 0 << 1 << 12;
  QTest::newRow("uppercase exponential") << QByteArray("120.12354E23") << true << false << TOKEN(NUMBER) << QVariant(120.12354E23)  << 1 << 0 << 1 << 12;
  QTest::newRow("negative exponential single digit") << QByteArray("-10e2") << true << false << TOKEN(NUMBER) << QVariant(-1000)  << 1 << 0 << 1 << 5;
  QTest::newRow("negative exponential multiple digits") << QByteArray("-10e23") << true << false << TOKEN(NUMBER) << QVariant(-10e23)  << 1 << 0 << 1 << 6;
  QTest::newRow("negative exponential zero") << QByteArray("-0e23") << true << false << TOKEN(NUMBER) << QVariant(0)  << 1 << 0 << 1 << 5;
  QTest::newRow("negative exponential fractional") << QByteArray("-0.12354e23") << true << false << TOKEN(NUMBER) << QVariant(-0.12354e23)  << 1 << 0 << 1 << 11;
  QTest::newRow("negative exponential fractional multiple digits") << QByteArray("-120.12354e23") << true << false << TOKEN(NUMBER) << QVariant(-120.12354e23)  << 1 << 0 << 1 << 13;
  QTest::newRow("negative exponent") << QByteArray("10e-2") << true << false << TOKEN(NUMBER) << QVariant(10e-2)  << 1 << 0 << 1 << 5;
  QTest::newRow("positive exponent with plus") << QByteArray("10e+2") << true << false << TOKEN(NUMBER) << QVariant(1000)  << 1 << 0 << 1 << 5;
  
  QTest::newRow("invalid multiple digits") << QByteArray("001") << true << false << TOKEN(NUMBER) << QVariant(0)  << 1 << 0 << 1 << 1;
  QTest::newRow("invalid negative multiple digits") << QByteArray("-001") << true << false << TOKEN(NUMBER) << QVariant(0)  << 1 << 0 << 1 << 2;
  QTest::newRow("invalid fractional") << QByteArray("12.") << true << true << TOKEN(INVALID) << QVariant(12)  << 1 << 2 << 1 << 3;
  QTest::newRow("invalid exponential 1") << QByteArray("-5e+") << true << true << TOKEN(INVALID) << QVariant(-5)  << 1 << 2 << 1 << 3;
  QTest::newRow("invalid exponential 2") << QByteArray("2e") << true << true << TOKEN(INVALID) << QVariant(2)  << 1 << 1 << 1 << 2;
  QTest::newRow("invalid exponential 3") << QByteArray("3e+") << true << true << TOKEN(INVALID) << QVariant(3)  << 1 << 1 << 1 << 2;
  QTest::newRow("invalid exponential 4") << QByteArray("4.3E") << true << true << TOKEN(INVALID) << QVariant(4.3)  << 1 << 3 << 1 << 4;
  QTest::newRow("invalid exponential 5") << QByteArray("5.4E-") << true << true << TOKEN(INVALID) << QVariant(5.4)  << 1 << 3 << 1 << 4;

  QTest::newRow("colon") << QByteArray(":") << true << false << TOKEN(COLON) << QVariant()  << 1 << 0 << 1 << 1;
  QTest::newRow("comma") << QByteArray(",") << true << false << TOKEN(COMMA) << QVariant()  << 1 << 0 << 1 << 1;
  QTest::newRow("square bracket open") << QByteArray("[") << true << false << TOKEN(SQUARE_BRACKET_OPEN) << QVariant()  << 1 << 0 << 1 << 1;
  QTest::newRow("square bracket close") << QByteArray("]") << true << false << TOKEN(SQUARE_BRACKET_CLOSE) << QVariant()  << 1 << 0 << 1 << 1;
  QTest::newRow("curly bracket open") << QByteArray("{") << true << false << TOKEN(CURLY_BRACKET_OPEN) << QVariant()  << 1 << 0 << 1 << 1;
  QTest::newRow("curly bracket close") << QByteArray("}") << true << false << TOKEN(CURLY_BRACKET_CLOSE) << QVariant()  << 1 << 0 << 1 << 1;

  QTest::newRow("too large unsinged number") << QByteArray("18446744073709551616") << false << false << TOKEN(INVALID) << QVariant(ULLONG_MAX) << 1 << 0 << 1 << 20;
  QTest::newRow("too large signed number") << QByteArray("-9223372036854775808") << false << false << TOKEN(INVALID) << QVariant(LLONG_MIN) << 1 << 0 << 1 << 20;
  QTest::newRow("too large exponential") << QByteArray("1.7976931348623157e309") << false << false << TOKEN(INVALID) << QVariant(0) << 1 << 0 << 1 << 22;
  QTest::newRow("not allowed nan") << QByteArray("nan") << false << false << TOKEN(INVALID) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("not allowed infinity") << QByteArray("Infinity") << false << false << TOKEN(INVALID) << QVariant() << 1 << 0 << 1 << 1;
  QTest::newRow("unknown") << QByteArray("*") << true << false << TOKEN(INVALID) << QVariant()  << 1 << 0 << 1 << 1;
}


void TestScanner::scanSpecialNumbers() {
  QFETCH(QByteArray, input);
  QFETCH(bool, isInfinity);
  QFETCH(bool, isNegative);
  QFETCH(bool, isNan);
  QFETCH(int, expectedLocationBeginLine);
  QFETCH(int, expectedLocationBeginColumn);
  QFETCH(int, expectedLocationEndLine);
  QFETCH(int, expectedLocationEndColumn);
  
  QBuffer buffer;
  buffer.open(QBuffer::ReadWrite);
  buffer.write(input);
  buffer.seek(0);  
  JSonScanner scanner(&buffer);
  scanner.allowSpecialNumbers(true);

  QVariant yylval;
  yy::position position(YY_NULL, 1, 0);
  yy::location location(position, position);
  int result = scanner.yylex(&yylval, &location);
  
  QCOMPARE(result, TOKEN(NUMBER));
  QVERIFY(yylval.type() == QVariant::Double);

  double doubleResult = yylval.toDouble();

  #if defined(Q_OS_SYMBIAN) || defined(Q_OS_ANDROID) || defined(Q_OS_BLACKBERRY)
    QCOMPARE(bool(isinf(doubleResult)), isInfinity);
  #else
    // skip this test for MSVC, because there is no "isinf" function.
    #ifndef Q_CC_MSVC
        QCOMPARE(bool(std::isinf(doubleResult)), isInfinity);
    #endif
  #endif

  QCOMPARE(doubleResult<0, isNegative);

  #if defined(Q_OS_SYMBIAN) || defined(Q_OS_ANDROID) || defined(Q_OS_BLACKBERRY)
      QCOMPARE(bool(isnan(doubleResult)), isNan);
  #else
    // skip this test for MSVC, because there is no "isinf" function.
    #ifndef Q_CC_MSVC
        QCOMPARE(bool(std::isnan(doubleResult)), isNan);
    #endif
  #endif
    
  QCOMPARE(location.begin.line, (uint)expectedLocationBeginLine);
  QCOMPARE(location.begin.column, (uint)expectedLocationBeginColumn);
  QCOMPARE(location.end.line, (uint)expectedLocationEndLine);
  QCOMPARE(location.end.column, (uint)expectedLocationEndColumn);
}

void TestScanner::scanSpecialNumbers_data() {
  QTest::addColumn<QByteArray>("input");
  QTest::addColumn<bool>("isInfinity");
  QTest::addColumn<bool>("isNegative");
  QTest::addColumn<bool>("isNan");
  QTest::addColumn<int>("expectedLocationBeginLine");
  QTest::addColumn<int>("expectedLocationBeginColumn");
  QTest::addColumn<int>("expectedLocationEndLine");
  QTest::addColumn<int>("expectedLocationEndColumn");
  
  QTest::newRow("nan") << QByteArray("nan") << false << false << true << 1 << 0 << 1 << 3;
  QTest::newRow("NAN") << QByteArray("NAN") << false << false << true << 1 << 0 << 1 << 3;
  QTest::newRow("NaN") << QByteArray("NaN") << false << false << true << 1 << 0 << 1 << 3;

  QTest::newRow("infinity") << QByteArray("infinity") << true << false << false << 1 << 0 << 1 << 8;
  QTest::newRow("Infinity") << QByteArray("infinity") << true << false << false << 1 << 0 << 1 << 8;

  QTest::newRow("-infinity") << QByteArray("-infinity") << true << true << false << 1 << 0 << 1 << 9;
  QTest::newRow("-Infinity") << QByteArray("-Infinity") << true << true << false << 1 << 0 << 1 << 9;
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// using Qt4 rather then Qt5
QTEST_MAIN(TestScanner)
#include "moc_testscanner.cxx"
#else
QTEST_GUILESS_MAIN(TestScanner)
#include "testscanner.moc"
#endif
