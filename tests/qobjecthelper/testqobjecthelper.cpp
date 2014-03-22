
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
#include <QtCore/QVariantList>

#include <QtTest/QtTest>

#include <QJson/Parser>
#include <QJson/Serializer>
#include <QJson/QObjectHelper>

#include "person.h"

class TestQObjectHelper: public QObject
{
  Q_OBJECT
  private slots:
    void testQObject2QVariant();
    void testQVariant2QObject();
};

using namespace QJson;

void TestQObjectHelper::testQObject2QVariant()
{
  QString name = QLatin1String("Flavio Castelli");
  int phoneNumber = 123456;
  Person::Gender gender = Person::Male;
  QDate dob (1982, 7, 12);
  QVariantList nicknames;
  nicknames << QLatin1String("nickname1") << QLatin1String("nickname2");
  quint16 luckyNumber = 123;

  Person person;
  person.setName(name);
  person.setPhoneNumber(phoneNumber);
  person.setGender(gender);
  person.setDob(dob);
  person.setCustomField(nicknames);
  person.setLuckyNumber(luckyNumber);

  QVariantMap expected;
  expected[QLatin1String("name")] = QVariant(name);
  expected[QLatin1String("phoneNumber")] = QVariant(phoneNumber);
  expected[QLatin1String("gender")] = QVariant(gender);
  expected[QLatin1String("dob")] = QVariant(dob);
  expected[QLatin1String("customField")] = nicknames;
  expected[QLatin1String("luckyNumber")] = luckyNumber;

  QVariantMap result = QObjectHelper::qobject2qvariant(&person);
  QCOMPARE(result, expected);
}

void TestQObjectHelper::testQVariant2QObject()
{
  bool ok;
  QString name = QLatin1String("Flavio Castelli");
  int phoneNumber = 123456;
  Person::Gender gender = Person::Male;
  QDate dob (1982, 7, 12);
  QVariantList nicknames;
  nicknames << QLatin1String("nickname1") << QLatin1String("nickname2");
  quint16 luckyNumber = 123;

  Person expected_person;
  expected_person.setName(name);
  expected_person.setPhoneNumber(phoneNumber);
  expected_person.setGender(gender);
  expected_person.setDob(dob);
  expected_person.setCustomField(nicknames);
  expected_person.setLuckyNumber(luckyNumber);

  QVariantMap variant = QObjectHelper::qobject2qvariant(&expected_person);

  Serializer serializer;
  QByteArray json = serializer.serialize(variant, &ok);
  qDebug() << "json is" << json;
  QVERIFY(ok);

  Parser parser;
  QVariant parsedVariant = parser.parse(json,&ok);
  QVERIFY(ok);
  qDebug() << parsedVariant;
  QVERIFY(parsedVariant.canConvert(QVariant::Map));

  Person person;
  QCOMPARE(Person::Female, person.gender());
  QObjectHelper::qvariant2qobject(parsedVariant.toMap(), &person);

  QCOMPARE(person.name(), name);
  QCOMPARE(person.phoneNumber(), phoneNumber);
  QCOMPARE(person.gender(), gender);
  QCOMPARE(person.dob(), dob);
  QCOMPARE(person.customField(), QVariant(nicknames));
  QCOMPARE(person.luckyNumber(), luckyNumber);
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// using Qt4 rather then Qt5
QTEST_MAIN(TestQObjectHelper)
#include "moc_testqobjecthelper.cxx"
#else
QTEST_GUILESS_MAIN(TestQObjectHelper)
#include "testqobjecthelper.moc"
#endif
