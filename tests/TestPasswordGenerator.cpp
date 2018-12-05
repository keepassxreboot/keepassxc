/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "TestPasswordGenerator.h"
#include "core/PasswordGenerator.h"
#include "crypto/Crypto.h"

#include <QRegularExpression>
#include <QTest>

QTEST_GUILESS_MAIN(TestPasswordGenerator)

void TestPasswordGenerator::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestPasswordGenerator::testCharClasses()
{
    PasswordGenerator generator;
    QVERIFY(!generator.isValid());
    generator.setCharClasses(PasswordGenerator::CharClass::LowerLetters);
    generator.setLength(16);
    QVERIFY(generator.isValid());
    QCOMPARE(generator.generatePassword().size(), 16);

    generator.setLength(2000);
    QString password = generator.generatePassword();
    QCOMPARE(password.size(), 2000);
    QRegularExpression regex(R"(^[a-z]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::UpperLetters);
    password = generator.generatePassword();
    regex.setPattern(R"(^[A-Z]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Numbers);
    password = generator.generatePassword();
    regex.setPattern(R"(^\d+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Braces);
    password = generator.generatePassword();
    regex.setPattern(R"(^[\(\)\[\]\{\}]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Punctuation);
    password = generator.generatePassword();
    regex.setPattern(R"(^[\.,:;]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Quotes);
    password = generator.generatePassword();
    regex.setPattern(R"(^["']+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Dashes);
    password = generator.generatePassword();
    regex.setPattern(R"(^[\-/\\_|]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Math);
    password = generator.generatePassword();
    regex.setPattern(R"(^[!\*\+\-<=>\?]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Logograms);
    password = generator.generatePassword();
    regex.setPattern(R"(^[#`~%&^$@]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::EASCII);
    password = generator.generatePassword();
    regex.setPattern(R"(^[^a-zA-Z0-9\.,:;"'\-/\\_|!\*\+\-<=>\?#`~%&^$@]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
                             | PasswordGenerator::CharClass::Braces);
    password = generator.generatePassword();
    regex.setPattern(R"(^[a-zA-Z\(\)\[\]\{\}]+$)");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::Quotes | PasswordGenerator::CharClass::Numbers
                             | PasswordGenerator::CharClass::Dashes);
    password = generator.generatePassword();
    regex.setPattern(R"(^["'\d\-/\\_|]+$)");
    QVERIFY(regex.match(password).hasMatch());
}

void TestPasswordGenerator::testLookalikeExclusion()
{
    PasswordGenerator generator;
    generator.setLength(2000);
    generator.setCharClasses(PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters);
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QCOMPARE(password.size(), 2000);

    generator.setFlags(PasswordGenerator::GeneratorFlag::ExcludeLookAlike);
    password = generator.generatePassword();
    QRegularExpression regex("^[^lI0]+$");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
                             | PasswordGenerator::CharClass::Numbers);
    password = generator.generatePassword();
    regex.setPattern("^[^lI01]+$");
    QVERIFY(regex.match(password).hasMatch());

    generator.setCharClasses(PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
                             | PasswordGenerator::CharClass::Numbers | PasswordGenerator::CharClass::EASCII);
    password = generator.generatePassword();
    regex.setPattern("^[^lI01﹒]+$");
    QVERIFY(regex.match(password).hasMatch());
}
