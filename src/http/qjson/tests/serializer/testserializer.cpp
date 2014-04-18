/* This file is part of QJson
 *
 * Copyright (C) 2009 Flavio Castelli <flavio.castelli@gmail.com>
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

#include <limits>

#include <QtCore/QVariant>

#include <QtTest/QtTest>

#include <QJson/Parser>
#include <QJson/Serializer>

class TestSerializer: public QObject
{
  Q_OBJECT
  private slots:
    void testReadWriteEmptyDocument();
    void testReadWrite();
    void testReadWrite_data();
    void testValueNull();
    void testValueString();
    void testValueString_data();
    void testValueStringList();
    void testValueStringList_data();
    void testValueHashMap();
    void testValueInteger();
    void testValueInteger_data();
    void testValueDouble();
    void testValueDouble_data();
    void testSetDoublePrecision();
    void testValueFloat();
    void testValueFloat_data();
    void testValueBoolean();
    void testValueBoolean_data();
    void testSpecialNumbers();
    void testSpecialNumbers_data();
    void testIndentation();
    void testIndentation_data();
    void testSerializetoQIODevice();
    void testSerializeWithoutOkParam();

  private:
    void valueTest( const QVariant& value, const QString& expectedRegExp, bool errorExpected = false );
    void valueTest( const QObject* object, const QString& expectedRegExp );
};

Q_DECLARE_METATYPE(QVariant)

using namespace QJson;

void TestSerializer::testReadWriteEmptyDocument()
{
  QByteArray json = "";
  Parser parser;
  bool ok;
  QVariant result = parser.parse( json, &ok );
  QVERIFY(!ok);
  QVERIFY( ! result.isValid() );
  Serializer serializer;
  const QByteArray serialized = serializer.serialize( result, &ok);
  QVERIFY( ok );
  QByteArray expected = "null";
  QCOMPARE(expected, serialized);
}

void TestSerializer::testReadWrite()
{
  QFETCH( QByteArray, json );
  Parser parser;
  bool ok;
  QVariant result = parser.parse( json, &ok );
  QVERIFY(ok);
  Serializer serializer;
  const QByteArray serialized = serializer.serialize( result, &ok);
  QVERIFY(ok);
  QVariant writtenThenRead = parser.parse( serialized, &ok );
  QVERIFY(ok);
  QCOMPARE( result, writtenThenRead );
}

void TestSerializer::testReadWrite_data()
{
    QTest::addColumn<QByteArray>( "json" );

    // array tests
    QTest::newRow( "empty array" ) << QByteArray("[]");
    QTest::newRow( "basic array" ) << QByteArray("[\"person\",\"bar\"]");
    QTest::newRow( "single int array" ) << QByteArray("[6]");
    QTest::newRow( "int array" ) << QByteArray("[6,5,6,7]");
    const QByteArray json = "[1,2.4, -100, -3.4, -5e+0, 2e0,3e+0,4.3E0,5.4E-0]";
    QTest::newRow( QByteArray("array of various numbers") ) << json;

    // document tests
    QTest::newRow( "empty object" ) << QByteArray("{}");
    QTest::newRow( "basic document" ) << QByteArray("{\"person\":\"bar\"}");
    QTest::newRow( "object with ints" ) << QByteArray("{\"person\":6}");
    const QByteArray json2 = "{ \"person\":\"bar\",\n\"number\" : 51.3 , \"array\":[\"item1\", 123]}";
    QTest::newRow( "complicated document" ) << json2;

    // more complex cases
    const QByteArray json3 = "[ {\"person\":\"bar\"},\n\"number\",51.3 , [\"item1\", 123]]";
    QTest::newRow( "complicated array" ) << json3;
}

void TestSerializer::testIndentation()
{
  QFETCH( QByteArray, json );
  QFETCH( QByteArray, expected_compact );
  QFETCH( QByteArray, expected_min );
  QFETCH( QByteArray, expected_med );
  QFETCH( QByteArray, expected_full );

  // parse
  Parser parser;
  bool ok;
  QVariant parsed = parser.parse( json, &ok );
  QVERIFY(ok);

  Serializer serializer;
  QVariant reparsed;
  QByteArray serialized;

  // serialize with indent compact and reparse
  serializer.setIndentMode(QJson::IndentCompact);
  serialized = serializer.serialize( parsed, &ok);
  QVERIFY(ok);
  QCOMPARE( serialized, expected_compact);
  reparsed = parser.parse( serialized, &ok);
  QVERIFY(ok);
  QCOMPARE( parsed, reparsed);

  // serialize with indent minimum and reparse
  serializer.setIndentMode(QJson::IndentMinimum);
  serialized = serializer.serialize( parsed, &ok);
  QVERIFY(ok);
  QCOMPARE( serialized, expected_min);
  reparsed = parser.parse( serialized, &ok);
  QVERIFY(ok);
  QCOMPARE( parsed, reparsed);

  // serialize with indent medium and reparse
  serializer.setIndentMode(QJson::IndentMedium);
  serialized = serializer.serialize( parsed, &ok);
  QVERIFY(ok);
  QCOMPARE( serialized, expected_med);
  reparsed = parser.parse( serialized, &ok );
  QVERIFY(ok);
  QCOMPARE( parsed, reparsed);

  // serialize with indent full and reparse
  serializer.setIndentMode(QJson::IndentFull);
  serialized = serializer.serialize( parsed, &ok);
  QVERIFY(ok);
  QCOMPARE( serialized, expected_full);
  reparsed = parser.parse( serialized, &ok );
  QVERIFY(ok);
  QCOMPARE( parsed, reparsed);
}

void TestSerializer::testIndentation_data()
{
    QTest::addColumn<QByteArray>( "json" );
    QTest::addColumn<QByteArray>( "expected_compact" );
    QTest::addColumn<QByteArray>( "expected_min" );
    QTest::addColumn<QByteArray>( "expected_med" );
    QTest::addColumn<QByteArray>( "expected_full" );
    const QByteArray json = " { \"foo\" : 0, \"foo1\" : 1, \"foo2\" : [ { \"bar\" : 1, \"foo\" : 0, \"foobar\" : 0 }, { \"bar\" : 1, \"foo\" : 1, \"foobar\" : 1 } ], \"foo3\" : [ 1, 2, 3, 4, 5, 6 ] }";
    const QByteArray ex_compact = "{\"foo\":0,\"foo1\":1,\"foo2\":[{\"bar\":1,\"foo\":0,\"foobar\":0},{\"bar\":1,\"foo\":1,\"foobar\":1}],\"foo3\":[1,2,3,4,5,6]}";
    const QByteArray ex_min = "{ \"foo\" : 0, \"foo1\" : 1, \"foo2\" : [\n  { \"bar\" : 1, \"foo\" : 0, \"foobar\" : 0 },\n  { \"bar\" : 1, \"foo\" : 1, \"foobar\" : 1 }\n ], \"foo3\" : [\n  1,\n  2,\n  3,\n  4,\n  5,\n  6\n ] }";
    const QByteArray ex_med = "{\n \"foo\" : 0, \"foo1\" : 1, \"foo2\" : [\n  {\n   \"bar\" : 1, \"foo\" : 0, \"foobar\" : 0\n  },\n  {\n   \"bar\" : 1, \"foo\" : 1, \"foobar\" : 1\n  }\n ], \"foo3\" : [\n  1,\n  2,\n  3,\n  4,\n  5,\n  6\n ]\n}";
    const QByteArray ex_full = "{\n \"foo\" : 0,\n \"foo1\" : 1,\n \"foo2\" : [\n  {\n   \"bar\" : 1,\n   \"foo\" : 0,\n   \"foobar\" : 0\n  },\n  {\n   \"bar\" : 1,\n   \"foo\" : 1,\n   \"foobar\" : 1\n  }\n ],\n \"foo3\" : [\n  1,\n  2,\n  3,\n  4,\n  5,\n  6\n ]\n}";
    QTest::newRow( "test indents" ) << json << ex_compact << ex_min << ex_med << ex_full;
}

void TestSerializer::valueTest( const QVariant& value, const QString& expectedRegExp, bool errorExpected )
{
  Serializer serializer;
  bool ok;
  const QByteArray serialized = serializer.serialize( value, &ok);
  QCOMPARE(ok, !errorExpected);
  QCOMPARE(serialized.isNull(), errorExpected);
  const QString serializedUnicode = QString::fromUtf8( serialized );
  if (!errorExpected) {
    QRegExp expected( expectedRegExp );
    QVERIFY( expected.isValid() );
    QVERIFY2( expected.exactMatch( serializedUnicode ),
      qPrintable( QString( QLatin1String( "Expected regexp \"%1\" but got \"%2\"." ) )
        .arg( expectedRegExp ).arg( serializedUnicode ) ) );
  } else {
    QVERIFY(!serializer.errorMessage().isEmpty());
  }
}

void TestSerializer::valueTest( const QObject* object, const QString& expectedRegExp )
{
  Serializer serializer;
  bool ok;
  const QByteArray serialized = serializer.serialize( object, &ok);
  QVERIFY(ok);
  const QString serializedUnicode = QString::fromUtf8( serialized );
  QRegExp expected( expectedRegExp );
  QVERIFY( expected.isValid() );
  QVERIFY2( expected.exactMatch( serializedUnicode ),
    qPrintable( QString( QLatin1String( "Expected regexp \"%1\" but got \"%2\"." ) )
      .arg( expectedRegExp ).arg( serializedUnicode ) ) );
}

void TestSerializer::testValueNull()
{
  valueTest( QVariant(), QLatin1String( "\\s*null\\s*" ) );
  QVariantMap map;
  map[QLatin1String("value")] = QVariant();
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:\\s*null\\s*\\}\\s*" ) );
}

void TestSerializer::testValueString()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  valueTest( value, expected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ) );
}

void TestSerializer::testValueString_data()
{
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );
  QTest::newRow( "null string" ) << QVariant( QString() ) << QString( QLatin1String( "\\s*\"\"\\s*" ) );
  QTest::newRow( "empty string" ) << QVariant( QString( QLatin1String( "" ) ) ) << QString( QLatin1String( "\\s*\"\"\\s*" ) );
  QTest::newRow( "Simple String" ) << QVariant( QString( QLatin1String( "simpleString" ) ) ) << QString( QLatin1String( "\\s*\"simpleString\"\\s*" ) );
  QTest::newRow( "string with tab" ) << QVariant( QString( QLatin1String( "string\tstring" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\tstring\"\\s*" ) );
  QTest::newRow( "string with newline" ) << QVariant( QString( QLatin1String( "string\nstring" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\nstring\"\\s*" ) );
  QTest::newRow( "string with bell" ) << QVariant( QString( QLatin1String( "string\bstring" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\bstring\"\\s*" ) );
  QTest::newRow( "string with return" ) << QVariant( QString( QLatin1String( "string\rstring" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\rstring\"\\s*" ) );
  QTest::newRow( "string with double quote" ) << QVariant( QString( QLatin1String( "string\"string" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\\"string\"\\s*" ) );
  QTest::newRow( "string with backslash" ) << QVariant( QString( QLatin1String( "string\\string" ) ) ) << QString( QLatin1String( "\\s*\"string\\\\\\\\string\"\\s*" ) );
  QString testStringWithUnicode = QString( QLatin1String( "string" ) ) + QChar( 0x2665 ) + QLatin1String( "string" );
  QString testEscapedString = QString( QLatin1String( "string" ) ) + QLatin1String("\\\\u2665") + QLatin1String( "string" );
  QTest::newRow( "string with unicode" ) << QVariant( testStringWithUnicode ) << QLatin1String( "\\s*\"" ) + testEscapedString + QLatin1String( "\"\\s*" );
}

void TestSerializer::testValueStringList()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  valueTest( value, expected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ) );
}

void TestSerializer::testValueStringList_data()
{
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );

  QStringList stringlist;
  QString expected;

  // simple QStringList
  stringlist << QLatin1String("hello") << QLatin1String("world");
  expected = QLatin1String( "\\s*\\[\\s*\"hello\"\\s*,\\s*\"world\"\\s*\\]\\s*" );
  QTest::newRow( "simple QStringList" ) << QVariant( stringlist) << expected;
}

void TestSerializer::testValueInteger()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  valueTest( value, expected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ) );
}

void TestSerializer::testValueInteger_data()
{
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );

  QTest::newRow( "int 0" ) << QVariant( static_cast<int>( 0 ) ) << QString( QLatin1String( "\\s*0\\s*" ) );
  QTest::newRow( "uint 0" ) << QVariant( static_cast<uint>( 0 ) ) << QString( QLatin1String( "\\s*0\\s*" ) );
  QTest::newRow( "int -1" ) << QVariant( static_cast<int>( -1 ) ) << QString( QLatin1String( "\\s*-1\\s*" ) );
  QTest::newRow( "int 2133149800" ) << QVariant( static_cast<int>(2133149800) ) << QString( QLatin1String( "\\s*2133149800\\s*" ) );
  QTest::newRow( "uint 4133149800" ) << QVariant( static_cast<uint>(4133149800u) ) << QString( QLatin1String( "\\s*4133149800\\s*" ) );
  QTest::newRow( "uint64 932838457459459" ) << QVariant( Q_UINT64_C(932838457459459) ) << QString( QLatin1String( "\\s*932838457459459\\s*" ) );
  QTest::newRow( "max unsigned long long" ) << QVariant( std::numeric_limits<unsigned long long>::max() ) << QString( QLatin1String( "\\s*%1\\s*" ) ).arg(std::numeric_limits<unsigned long long>::max());
}

void TestSerializer::testValueDouble()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  QFETCH( bool, errorExpected );
  valueTest( value, expected, errorExpected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ), errorExpected );
}

void TestSerializer::testValueDouble_data()
{
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );
  QTest::addColumn<bool>( "errorExpected" );

  QTest::newRow( "double 0" ) << QVariant( 0.0 ) << QString( QLatin1String( "\\s*0.0\\s*" ) ) << false;
  QTest::newRow( "double -1" ) << QVariant( -1.0 ) << QString( QLatin1String( "\\s*-1.0\\s*" ) ) << false;
  QTest::newRow( "double 1.5E-20" ) << QVariant( 1.5e-20 ) << QString( QLatin1String( "\\s*1.5[Ee]-20\\s*" ) ) << false;
  QTest::newRow( "double -1.5E-20" ) << QVariant( -1.5e-20 ) << QString( QLatin1String( "\\s*-1.5[Ee]-20\\s*" ) ) << false;
  QTest::newRow( "double 2.0E-20" ) << QVariant( 2.0e-20 ) << QString( QLatin1String( "\\s*2(?:.0)?[Ee]-20\\s*" ) ) << false;
  QTest::newRow( "double infinity" ) << QVariant( std::numeric_limits< double >::infinity() ) << QString( ) << true;
  QTest::newRow( "double -infinity" ) << QVariant( -std::numeric_limits< double >::infinity() ) << QString( ) << true;
  QTest::newRow( "double NaN" ) << QVariant( std::numeric_limits< double >::quiet_NaN() ) << QString( ) << true;
}

void TestSerializer::testSetDoublePrecision()
{
  bool ok;
  Serializer serializer;
  QByteArray actual;
  QString    expected, actualUnicode;

  double num = 0.12345678;

  // Set 1 as double precision
  serializer.setDoublePrecision(1);
  expected      = QString(QLatin1String("0.1"));
  actual        = serializer.serialize( QVariant(num), &ok);
  QVERIFY(ok);
  actualUnicode = QString::fromUtf8(actual);

  QVERIFY2( QString::compare(expected, actualUnicode ) == 0,
            qPrintable( QString( QLatin1String( "Expected \"%1\" but got \"%2\"." ) )
          .arg( expected ).arg( actualUnicode ) ) );

  // Set 2 as double precision
  serializer.setDoublePrecision(2);
  expected      = QString(QLatin1String("0.12"));
  actual        = serializer.serialize( QVariant(num), &ok);
  QVERIFY(ok);
  actualUnicode = QString::fromUtf8(actual);

  QVERIFY2( QString::compare(expected, actualUnicode ) == 0,
            qPrintable( QString( QLatin1String( "Expected \"%1\" but got \"%2\"." ) )
          .arg( expected ).arg( actualUnicode ) ) );

  // Set 4 as double precision
  serializer.setDoublePrecision(4);
  expected      = QString(QLatin1String("0.1235"));
  actual        = serializer.serialize( QVariant(num), &ok);
  QVERIFY(ok);
  actualUnicode = QString::fromUtf8(actual);

  QVERIFY2( QString::compare(expected, actualUnicode ) == 0,
            qPrintable( QString( QLatin1String( "Expected \"%1\" but got \"%2\"." ) )
          .arg( expected ).arg( actualUnicode ) ) );

  // Set 14 as double precision
  serializer.setDoublePrecision(14);
  expected      = QString(QLatin1String("0.12345678"));
  actual        = serializer.serialize( QVariant(num), &ok);
  QVERIFY(ok);
  actualUnicode = QString::fromUtf8(actual);

  QVERIFY2( QString::compare(expected, actualUnicode ) == 0,
            qPrintable( QString( QLatin1String( "Expected \"%1\" but got \"%2\"." ) )
          .arg( expected ).arg( actualUnicode ) ) );
}

void TestSerializer::testValueFloat()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  QFETCH( bool, errorExpected );
  valueTest( value, expected, errorExpected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ), errorExpected );
}

void TestSerializer::testValueFloat_data()
{
  QVariant v;
  float value;

  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );
  QTest::addColumn<bool>( "errorExpected" );

  value = 0;
  v.setValue(value);
  QTest::newRow( "float 0" ) << v << QString( QLatin1String( "\\s*0.0\\s*" ) ) << false;

  value = -1;
  v.setValue(value);
  QTest::newRow( "float -1" ) << v << QString( QLatin1String( "\\s*-1.0\\s*" ) ) << false;

  value = 1.12f;
  v.setValue(value);
  QTest::newRow( "float 1.12" ) << v << QString( QLatin1String( "\\s*1.12\\s*" ) ) << false;
}

void TestSerializer::testValueBoolean()
{
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  valueTest( value, expected );

  QVariantMap map;
  map[QLatin1String("value")] = value;
  valueTest( QVariant(map), QLatin1String( "\\s*\\{\\s*\"value\"\\s*:" ) + expected + QLatin1String( "\\}\\s*" ) );
}

void TestSerializer::testValueBoolean_data()
{
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );

  QTest::newRow( "bool false" ) << QVariant( false ) << QString( QLatin1String( "\\s*false\\s*" ) );
  QTest::newRow( "bool true" ) << QVariant( true ) << QString( QLatin1String( "\\s*true\\s*" ) );
}

void TestSerializer::testSpecialNumbers() {
  bool ok;
  QFETCH( QVariant, value );
  QFETCH( QString, expected );
  Serializer specialSerializer;
  QVERIFY(!specialSerializer.specialNumbersAllowed());
  specialSerializer.allowSpecialNumbers(true);
  QVERIFY(specialSerializer.specialNumbersAllowed());
  QByteArray serialized = specialSerializer.serialize(value, &ok);
  QVERIFY(ok);
  QCOMPARE(QString::fromLocal8Bit(serialized), expected);
}

void TestSerializer::testSpecialNumbers_data() {
  QTest::addColumn<QVariant>( "value" );
  QTest::addColumn<QString>( "expected" );

  QTest::newRow( "Infinity" ) << QVariant( std::numeric_limits< double >::infinity() ) << QString::fromLocal8Bit("Infinity");
  QTest::newRow( "-Infinity" ) << QVariant( -std::numeric_limits< double >::infinity() ) << QString::fromLocal8Bit("-Infinity");
  QTest::newRow( "Infinity" ) <<  QVariant( std::numeric_limits< double >::quiet_NaN() ) << QString::fromLocal8Bit("NaN");
}

void TestSerializer::testSerializetoQIODevice() {
  QBuffer buffer;
  QVariantList variant;
  variant << QVariant(QLatin1String("Hello"));
  variant << QVariant(QLatin1String("world!"));

  Serializer serializer;
  bool ok;

  serializer.serialize(variant, &buffer, &ok);

  QCOMPARE(QString(QLatin1String(buffer.data())),
           QString(QLatin1String("[ \"Hello\", \"world!\" ]")));
  QVERIFY(ok);
}

void TestSerializer::testSerializeWithoutOkParam() {
  QBuffer buffer;
  QVariantList variant;
  variant << QVariant(QLatin1String("Hello"));
  variant << QVariant(QLatin1String("world!"));

  Serializer serializer;

  const QByteArray serialized = serializer.serialize(variant);
  const QByteArray expected = "[ \"Hello\", \"world!\" ]";
  QCOMPARE(expected, serialized);


  // test a serialization which produces an error
  QVariant brokenVariant ( std::numeric_limits< double >::quiet_NaN() );
  QVERIFY(serializer.serialize(brokenVariant).isEmpty());
}

void TestSerializer::testValueHashMap()
{
  Serializer serializer;
  bool ok;

  QVariantHash hash;
  hash[QLatin1String("one")]   = 1;
  hash[QLatin1String("three")] = 3;
  hash[QLatin1String("seven")] = 7;

  QByteArray json = serializer.serialize(hash, &ok);
  QVERIFY(ok);

  Parser parser;
  QVariant var = parser.parse(json, &ok);
  QVERIFY(ok);

  QVariantMap vmap = var.toMap();
  QHashIterator<QString, QVariant> hIt( hash );
  while ( hIt.hasNext() ) {
    hIt.next();
    QString key = hIt.key();
    QVariant value = hIt.value();

    QMap<QString, QVariant>::const_iterator mIt = vmap.constFind(key);
    QVERIFY(mIt != vmap.constEnd());
    QCOMPARE(mIt.value(), value);
  }

}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// using Qt4 rather then Qt5
QTEST_MAIN(TestSerializer)
#include "moc_testserializer.cxx"
#else
QTEST_GUILESS_MAIN(TestSerializer)
#include "testserializer.moc"
#endif
