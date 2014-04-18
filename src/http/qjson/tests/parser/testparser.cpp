/* This file is part of QJson
 *
 * Copyright (C) 2008 Flavio Castelli <flavio.castelli@gmail.com>
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

#include <QtTest/QtTest>

#include <QJson/Parser>
#include <QJson/Serializer>

#include <QLocale>

class TestParser: public QObject
{
  Q_OBJECT
  private slots:
    void parseNonAsciiString();
    void parseSimpleObject();
    void parseEmptyObject();
    void parseEmptyValue();
    void parseUrl();
    void parseMultipleObject();

    void parseSimpleArray();
    void parseInvalidObject();
    void parseInvalidObject_data();
    void parseMultipleArray();

    void reuseSameParser();

    void testTrueFalseNullValues();
    void testEscapeChars();
    void testNumbers();
    void testNumbers_data();
    void testDoubleParsingWithDifferentLocale();
    void testTopLevelValues();
    void testTopLevelValues_data();
    void testReadWrite();
    void testReadWrite_data();
};

Q_DECLARE_METATYPE(QVariant)
Q_DECLARE_METATYPE(QVariant::Type)

using namespace QJson;

void TestParser::parseSimpleObject() {
  QByteArray json = "{\"foo\":\"bar\"}";
  QVariantMap map;
  map.insert (QLatin1String("foo"), QLatin1String("bar"));
  QVariant expected(map);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

void TestParser::parseEmptyObject() {
  QByteArray json = "{}";
  QVariantMap map;
  QVariant expected (map);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

void TestParser::parseEmptyValue() {
  QByteArray json = "{\"value\": \"\"}";

  QVariantMap map;
  map.insert (QLatin1String("value"), QString(QLatin1String("")));
  QVariant expected (map);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
  QVERIFY (result.toMap().value(QLatin1String("value")).type() == QVariant::String);

  QString value = result.toMap().value(QLatin1String("value")).toString();
  QVERIFY (value.isEmpty());
}

void TestParser::parseInvalidObject() {
  QFETCH(QByteArray, json);

  Parser parser;
  bool ok;
  parser.parse (json, &ok);
  QVERIFY (!ok);
  QVERIFY(!parser.errorString().isEmpty());
}

void TestParser::parseInvalidObject_data() {
  QTest::addColumn<QByteArray>("json");

  QTest::newRow("unclosed object") <<  QByteArray("{\"foo\":\"bar\"");
  QTest::newRow("infinum (disallow") << QByteArray("Infinum");
  QTest::newRow("Nan (disallow") << QByteArray("NaN");
  QTest::newRow("no data") << QByteArray("");
}


void TestParser::parseNonAsciiString() {
  QByteArray json = "{\"artist\":\"Queensr\\u00ffche\"}";
  QVariantMap map;

  QChar unicode_char (0x00ff);
  QString unicode_string;
  unicode_string.setUnicode(&unicode_char, 1);
  unicode_string = QLatin1String("Queensr") + unicode_string + QLatin1String("che");

  map.insert (QLatin1String("artist"), unicode_string);
  QVariant expected (map);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

void TestParser::parseMultipleObject() {
  //put also some extra spaces inside the json string
  QByteArray json = "{ \"foo\":\"bar\",\n\"number\" : 51.3 , \"array\":[\"item1\", 123]}";
  QVariantMap map;
  map.insert (QLatin1String("foo"), QLatin1String("bar"));
  map.insert (QLatin1String("number"), 51.3);
  QVariantList list;
  list.append (QLatin1String("item1"));
  list.append (QLatin1String("123"));
  map.insert (QLatin1String("array"), list);
  QVariant expected (map);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
  QVERIFY (result.toMap().value(QLatin1String("number")).canConvert<float>());
  QVERIFY (result.toMap().value(QLatin1String("array")).canConvert<QVariantList>());
}

void TestParser::parseUrl(){
  //"http:\/\/www.last.fm\/venue\/8926427"
  QByteArray json = "[\"http:\\/\\/www.last.fm\\/venue\\/8926427\"]";
  QVariantList list;
  list.append (QVariant(QLatin1String("http://www.last.fm/venue/8926427")));
  QVariant expected (list);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

 void TestParser::parseSimpleArray() {
  QByteArray json = "[\"foo\",\"bar\"]";
  QVariantList list;
  list.append (QLatin1String("foo"));
  list.append (QLatin1String("bar"));
  QVariant expected (list);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

void TestParser::parseMultipleArray() {
  //put also some extra spaces inside the json string
  QByteArray json = "[ {\"foo\":\"bar\"},\n\"number\",51.3 , [\"item1\", 123]]";
  QVariantMap map;
  map.insert (QLatin1String("foo"), QLatin1String("bar"));

  QVariantList array;
  array.append (QLatin1String("item1"));
  array.append (123);

  QVariantList list;
  list.append (map);
  list.append (QLatin1String("number"));
  list.append (QLatin1String("51.3"));
  list.append ((QVariant) array);

  QVariant expected (list);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
}

void TestParser::testTrueFalseNullValues() {
  QByteArray json = "[true,false, null, {\"foo\" : true}]";
  QVariantList list;
  list.append (QVariant(true));
  list.append (QVariant(false));
  list.append (QVariant());
  QVariantMap map;
  map.insert (QLatin1String("foo"), true);
  list.append (map);
  QVariant expected (list);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result, expected);
  QCOMPARE (result.toList().at(0).toBool(), true);
  QCOMPARE (result.toList().at(1).toBool(), false);
  QVERIFY (result.toList().at(2).isNull());
}

void TestParser::testEscapeChars() {
  QByteArray json = "[\"\\b \\f \\n \\r \\t \", \" \\\\ \\/ \\\\\", \"http:\\/\\/foo.com\"]";

  QVariantList list;
  list.append (QLatin1String("\b \f \n \r \t "));
  list.append (QLatin1String(" \\ / \\"));
  list.append (QLatin1String("http://foo.com"));

  QVariant expected (list);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (json, &ok);
  QVERIFY (ok);
  QCOMPARE(result.toList().size(), expected.toList().size() );
  QCOMPARE(result, expected);
}

void TestParser::testNumbers() {
  QFETCH(QByteArray, input);
  QFETCH(QVariant, expected);
  QFETCH(QVariant::Type, type);

  Parser parser;
  bool ok;
  QVariant result = parser.parse ('[' + input + ']', &ok);
  QVERIFY (ok);

  QVariant value = result.toList().at(0);
  QCOMPARE(value, expected);
  QCOMPARE( value.type(), type);
}

void TestParser::testNumbers_data() {
  QTest::addColumn<QByteArray>( "input" );
  QTest::addColumn<QVariant>( "expected" );
  QTest::addColumn<QVariant::Type>( "type" );

  QByteArray input;
  QVariant output;

  // simple ulonglong
  input = QByteArray("1");
  output = QVariant(QVariant::ULongLong);
  output.setValue(1);

  QTest::newRow("simple ulonglong") << input << output << QVariant::ULongLong;

  // big number
  input = QByteArray("12345678901234567890");
  output = QVariant(QVariant::ULongLong);
  output.setValue(12345678901234567890ull);

  QTest::newRow("big number") << input << output << QVariant::ULongLong;

  // simple double
  input = QByteArray("2.004");
  output = QVariant(QVariant::Double);
  output.setValue(2.004);

  QTest::newRow("simple double") << input << output << QVariant::Double;

  // negative int
  input = QByteArray("-100");
  output = QVariant(QVariant::LongLong);
  output.setValue(-100);

  QTest::newRow("negative int") << input << output << QVariant::LongLong;

  // negative double
  input = QByteArray("-3.4");
  output = QVariant(QVariant::Double);
  output.setValue(-3.4);

  QTest::newRow("negative double") << input << output << QVariant::Double;
}

void TestParser::testTopLevelValues() {
  QFETCH(QByteArray, input);
  QFETCH(QVariant, expected);
  QFETCH(QVariant::Type, type);

  Parser parser;
  bool ok;
  QVariant result = parser.parse (input, &ok);
  QVERIFY (ok);

  QCOMPARE(result, expected);
  QCOMPARE(result.type(), type);
}

void TestParser::testTopLevelValues_data() {
  QTest::addColumn<QByteArray>( "input" );
  QTest::addColumn<QVariant>( "expected" );
  QTest::addColumn<QVariant::Type>( "type" );

  QByteArray input;
  QVariant output;

  // string
  input = QByteArray("\"foo bar\"");
  output = QVariant(QLatin1String("foo bar"));
  QTest::newRow("string") << input << output << QVariant::String;

  // number
  input = QByteArray("2.4");
  output = QVariant(QVariant::Double);
  output.setValue(2.4);
  QTest::newRow("simple double") << input << output << QVariant::Double;

  // boolean
  input = QByteArray("true");
  output = QVariant(QVariant::Bool);
  output.setValue(true);
  QTest::newRow("bool") << input << output << QVariant::Bool;

  // null
  input = QByteArray("null");
  output = QVariant();
  QTest::newRow("null") << input << output << QVariant::Invalid;

  // array
  input = QByteArray("[1,2,3]");
  QVariantList list;
  list << QVariant(1) << QVariant(2) << QVariant(3);
  output = QVariant(QVariant::List);
  output.setValue(list);
  QTest::newRow("array") << input << output << QVariant::List;

  // object
  input = QByteArray("{\"foo\" : \"bar\"}");
  QVariantMap map;
  map.insert(QLatin1String("foo"), QLatin1String("bar"));
  output = QVariant(QVariant::Map);
  output.setValue(map);
  QTest::newRow("object") << input << output << QVariant::Map;
}

void TestParser::testDoubleParsingWithDifferentLocale() {
  QLocale oldLocale;
  QLocale itLocale(QLatin1String("it_IT.utf8"));

  QCOMPARE(itLocale.name(), QLatin1String("it_IT") );

  // the Italian locale uses ',' as digit separator.
  QLocale::setDefault(itLocale);

  Parser parser;
  bool ok;
  QVariant result = parser.parse ("12.3", &ok);
  QVERIFY (ok);

  QCOMPARE(result.toDouble(), 12.3);

  QLocale::setDefault(oldLocale);
}

void TestParser::testReadWrite()
{
  QFETCH( QVariant, variant );
  Serializer serializer;
  bool ok;

  QByteArray json = serializer.serialize(variant, &ok);
  QVERIFY(ok);

  Parser parser;
  QVariant result = parser.parse( json, &ok );
  QVERIFY(ok);
  QCOMPARE( result, variant );
}

void TestParser::testReadWrite_data()
{
    QTest::addColumn<QVariant>( "variant" );

    // array tests
    QTest::newRow( "empty array" ) << QVariant(QVariantList());

    // basic array
    QVariantList list;
    list << QString(QLatin1String("hello"));
    list << 12; 
    QTest::newRow( "basic array" ) << QVariant(list);

    // simple map 
    QVariantMap map;
    map[QString(QLatin1String("Name"))] = 32;
    QTest::newRow( "complicated array" ) << QVariant(map);
}

void TestParser::reuseSameParser()
{
  Parser parser;
  bool ok;

  parser.parse ("12.3", &ok);
  QVERIFY (ok);

  parser.parse ("wrong entry", &ok);
  QVERIFY (!ok);

  parser.parse ("12.3", &ok);
  QVERIFY (ok);
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// using Qt4 rather then Qt5
QTEST_MAIN(TestParser)
#include "moc_testparser.cxx"
#else
QTEST_GUILESS_MAIN(TestParser)
#include "testparser.moc"
#endif
