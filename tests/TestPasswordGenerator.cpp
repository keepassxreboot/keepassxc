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
#include "crypto/Crypto.h"

#include <QRegularExpression>
#include <QTest>

QTEST_GUILESS_MAIN(TestPasswordGenerator)

Q_DECLARE_METATYPE(PasswordGenerator::CharClasses)
Q_DECLARE_METATYPE(PasswordGenerator::GeneratorFlags)

namespace
{
    PasswordGenerator::CharClasses to_flags(PasswordGenerator::CharClass x)
    {
        return x;
    }

    PasswordGenerator::GeneratorFlags to_flags(PasswordGenerator::GeneratorFlag x)
    {
        return x;
    }
} // namespace

void TestPasswordGenerator::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestPasswordGenerator::init()
{
    m_generator.reset();
}

void TestPasswordGenerator::testCustomCharacterSet_data()
{
    QTest::addColumn<PasswordGenerator::CharClasses>("activeCharacterClasses");
    QTest::addColumn<QString>("customCharacterSet");
    QTest::addColumn<QRegularExpression>("expected");

    QTest::addRow("With active classes") << to_flags(PasswordGenerator::CharClass::UpperLetters) << "abc"
                                         << QRegularExpression("^[abcA-Z]{2000}$");
    QTest::addRow("Without any active class")
        << to_flags(PasswordGenerator::CharClass::NoClass) << "abc" << QRegularExpression("^[abc]{2000}$");
}

void TestPasswordGenerator::testCustomCharacterSet()
{
    QFETCH(PasswordGenerator::CharClasses, activeCharacterClasses);
    QFETCH(QString, customCharacterSet);
    QFETCH(QRegularExpression, expected);

    m_generator.setCharClasses(activeCharacterClasses);
    m_generator.setCustomCharacterSet(customCharacterSet);
    m_generator.setLength(2000);

    QVERIFY(m_generator.isValid());
    QString password = m_generator.generatePassword();
    QCOMPARE(password.size(), 2000);
    QVERIFY(expected.match(password).hasMatch());
}

void TestPasswordGenerator::testCharClasses_data()
{
    QTest::addColumn<PasswordGenerator::CharClasses>("activeCharacterClasses");
    QTest::addColumn<QRegularExpression>("expected");

    QTest::addRow("Lower Letters") << to_flags(PasswordGenerator::CharClass::LowerLetters)
                                   << QRegularExpression(R"(^[a-z]{2000}$)");
    QTest::addRow("Upper Letters") << to_flags(PasswordGenerator::CharClass::UpperLetters)
                                   << QRegularExpression(R"(^[A-Z]{2000}$)");
    QTest::addRow("Numbers") << to_flags(PasswordGenerator::CharClass::Numbers) << QRegularExpression(R"(^\d{2000}$)");
    QTest::addRow("Braces") << to_flags(PasswordGenerator::CharClass::Braces)
                            << QRegularExpression(R"(^[\(\)\[\]\{\}]{2000}$)");
    QTest::addRow("Punctuation") << to_flags(PasswordGenerator::CharClass::Punctuation)
                                 << QRegularExpression(R"(^[\.,:;]{2000}$)");
    QTest::addRow("Quotes") << to_flags(PasswordGenerator::CharClass::Quotes) << QRegularExpression(R"(^["']{2000}$)");
    QTest::addRow("Dashes") << to_flags(PasswordGenerator::CharClass::Dashes)
                            << QRegularExpression(R"(^[\-/\\_|]{2000}$)");
    QTest::addRow("Math") << to_flags(PasswordGenerator::CharClass::Math) << QRegularExpression(R"(^[!\*\+\-<=>\?]+$)");
    QTest::addRow("Logograms") << to_flags(PasswordGenerator::CharClass::Logograms)
                               << QRegularExpression(R"(^[#`~%&^$@]{2000}$)");
    QTest::addRow("Extended ASCII") << to_flags(PasswordGenerator::CharClass::EASCII)
                                    << QRegularExpression(R"(^[^a-zA-Z0-9\.,:;"'\-/\\_|!\*\+\-<=>\?#`~%&^$@]{2000}$)");
    QTest::addRow("Combinations 1") << (PasswordGenerator::CharClass::LowerLetters
                                        | PasswordGenerator::CharClass::UpperLetters
                                        | PasswordGenerator::CharClass::Braces)
                                    << QRegularExpression(R"(^[a-zA-Z\(\)\[\]\{\}]{2000}$)");
    QTest::addRow("Combinations 2") << (PasswordGenerator::CharClass::Quotes | PasswordGenerator::CharClass::Numbers
                                        | PasswordGenerator::CharClass::Dashes)
                                    << QRegularExpression(R"(^["'\d\-/\\_|]{2000}$)");
}

void TestPasswordGenerator::testCharClasses()
{

    QFETCH(PasswordGenerator::CharClasses, activeCharacterClasses);
    QFETCH(QRegularExpression, expected);

    m_generator.setCharClasses(activeCharacterClasses);
    m_generator.setLength(2000);

    QVERIFY(m_generator.isValid());
    QString password = m_generator.generatePassword();
    QCOMPARE(password.size(), 2000);
    QVERIFY(expected.match(password).hasMatch());
}

void TestPasswordGenerator::testLookalikeExclusion_data()
{
    QTest::addColumn<PasswordGenerator::CharClasses>("activeCharacterClasses");
    QTest::addColumn<QRegularExpression>("expected");
    QTest::addRow("Upper Letters") << (PasswordGenerator::CharClass::LowerLetters
                                       | PasswordGenerator::CharClass::UpperLetters)
                                   << QRegularExpression("^[^lBGIO]{2000}$");

    QTest::addRow("Letters and Numbers") << (PasswordGenerator::CharClass::LowerLetters
                                             | PasswordGenerator::CharClass::UpperLetters
                                             | PasswordGenerator::CharClass::Numbers)
                                         << QRegularExpression("^[^lBGIO0168]{2000}$");

    QTest::addRow("Letters, Numbers and extended ASCII")
        << (PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
            | PasswordGenerator::CharClass::Numbers | PasswordGenerator::CharClass::EASCII)
        << QRegularExpression("^[^lBGIO0168﹒]{2000}$");
}

void TestPasswordGenerator::testLookalikeExclusion()
{
    QFETCH(PasswordGenerator::CharClasses, activeCharacterClasses);
    QFETCH(QRegularExpression, expected);

    m_generator.setFlags(PasswordGenerator::ExcludeLookAlike);
    m_generator.setCharClasses(activeCharacterClasses);
    m_generator.setLength(2000);

    QVERIFY(m_generator.isValid());
    QString password = m_generator.generatePassword();
    QCOMPARE(password.size(), 2000);
    QVERIFY(expected.match(password).hasMatch());
}

void TestPasswordGenerator::testValidity_data()
{
    QTest::addColumn<PasswordGenerator::CharClasses>("activeCharacterClasses");
    QTest::addColumn<PasswordGenerator::GeneratorFlags>("generatorFlags");
    QTest::addColumn<QString>("customCharacterSet");
    QTest::addColumn<QString>("excludedCharacters");
    QTest::addColumn<int>("length");
    QTest::addColumn<bool>("isValid");

    QTest::addRow("No active class") << to_flags(PasswordGenerator::CharClass::NoClass)
                                     << PasswordGenerator::GeneratorFlags() << QString() << QString()
                                     << PasswordGenerator::DefaultLength << false;
    QTest::addRow("0 length") << to_flags(PasswordGenerator::CharClass::DefaultCharset)
                              << PasswordGenerator::GeneratorFlags() << QString() << QString() << 0 << false;
    QTest::addRow("All active classes excluded")
        << to_flags(PasswordGenerator::CharClass::Numbers) << PasswordGenerator::GeneratorFlags() << QString()
        << QString("0123456789") << PasswordGenerator::DefaultLength << false;
    QTest::addRow("All active classes excluded")
        << to_flags(PasswordGenerator::CharClass::NoClass) << PasswordGenerator::GeneratorFlags() << QString()
        << QString("0123456789") << PasswordGenerator::DefaultLength << false;
    QTest::addRow("One from every class with too few classes")
        << (PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString() << QString() << 1 << false;
    QTest::addRow("One from every class with excluded classes")
        << (PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
            | PasswordGenerator::CharClass::Numbers)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString() << QString("0123456789") << 2
        << true;
    QTest::addRow("Defaults valid") << to_flags(PasswordGenerator::CharClass::DefaultCharset)
                                    << to_flags(PasswordGenerator::GeneratorFlag::DefaultFlags)
                                    << PasswordGenerator::DefaultCustomCharacterSet
                                    << PasswordGenerator::DefaultExcludedChars << PasswordGenerator::DefaultLength
                                    << true;
    QTest::addRow("No active classes but custom charset")
        << to_flags(PasswordGenerator::CharClass::NoClass) << to_flags(PasswordGenerator::GeneratorFlag::DefaultFlags)
        << QString("a") << QString() << 1 << true;
}

void TestPasswordGenerator::testValidity()
{
    QFETCH(PasswordGenerator::CharClasses, activeCharacterClasses);
    QFETCH(PasswordGenerator::GeneratorFlags, generatorFlags);
    QFETCH(QString, customCharacterSet);
    QFETCH(QString, excludedCharacters);
    QFETCH(int, length);
    QFETCH(bool, isValid);

    m_generator.setCharClasses(activeCharacterClasses);
    m_generator.setFlags(generatorFlags);
    m_generator.setCustomCharacterSet(customCharacterSet);
    m_generator.setExcludedCharacterSet(excludedCharacters);
    m_generator.setLength(length);
    QCOMPARE(m_generator.isValid(), isValid);
}

void TestPasswordGenerator::testMinLength_data()
{
    QTest::addColumn<PasswordGenerator::CharClasses>("activeCharacterClasses");
    QTest::addColumn<PasswordGenerator::GeneratorFlags>("generatorFlags");
    QTest::addColumn<QString>("customCharacterSet");
    QTest::addColumn<QString>("excludedCharacters");
    QTest::addColumn<int>("expectedMinLength");

    QTest::addRow("No restriction without charsFromEveryGroup")
        << to_flags(PasswordGenerator::CharClass::Numbers)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup)
        << PasswordGenerator::DefaultCustomCharacterSet << PasswordGenerator::DefaultExcludedChars << 1;

    QTest::addRow("Min length should equal number of active classes")
        << (PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
            | PasswordGenerator::CharClass::Numbers)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString() << QString() << 3;
    QTest::addRow("Classes fully excluded by excluded characters do not count towards min length")
        << (PasswordGenerator::CharClass::Numbers | PasswordGenerator::LowerLetters
            | PasswordGenerator::CharClass::UpperLetters)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString() << QString("0123456789") << 2;

    QTest::addRow("Custom charset counts as class")
        << to_flags(PasswordGenerator::CharClass::UpperLetters)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString("a") << QString() << 2;
    QTest::addRow("Custom characters count even if included by an active class already")
        << (PasswordGenerator::CharClass::LowerLetters | PasswordGenerator::CharClass::UpperLetters
            | PasswordGenerator::CharClass::Numbers)
        << to_flags(PasswordGenerator::GeneratorFlag::CharFromEveryGroup) << QString("012345") << QString() << 4;
}

void TestPasswordGenerator::testMinLength()
{
    QFETCH(PasswordGenerator::CharClasses, activeCharacterClasses);
    QFETCH(PasswordGenerator::GeneratorFlags, generatorFlags);
    QFETCH(QString, customCharacterSet);
    QFETCH(QString, excludedCharacters);
    QFETCH(int, expectedMinLength);

    m_generator.setCharClasses(activeCharacterClasses);
    m_generator.setFlags(generatorFlags);
    m_generator.setCustomCharacterSet(customCharacterSet);
    m_generator.setExcludedCharacterSet(excludedCharacters);
    QCOMPARE(m_generator.getMinLength(), expectedMinLength);
}

void TestPasswordGenerator::testReset()
{
    PasswordGenerator default_generator;

    // Modify generator
    m_generator.setCharClasses(PasswordGenerator::CharClass::NoClass);
    m_generator.setFlags(PasswordGenerator::GeneratorFlag::NoFlags);
    m_generator.setCustomCharacterSet("avc");
    m_generator.setExcludedCharacterSet("asdv");
    m_generator.setLength(m_generator.getLength() + 1);

    Q_ASSERT(m_generator.getActiveClasses() != default_generator.getActiveClasses());
    Q_ASSERT(m_generator.getFlags() != default_generator.getFlags());
    Q_ASSERT(m_generator.getCustomCharacterSet() != default_generator.getCustomCharacterSet());
    Q_ASSERT(m_generator.getExcludedCharacterSet() != default_generator.getExcludedCharacterSet());

    m_generator.reset();
    QCOMPARE(m_generator.getActiveClasses(), default_generator.getActiveClasses());
    QCOMPARE(m_generator.getFlags(), default_generator.getFlags());
    QCOMPARE(m_generator.getCustomCharacterSet(), default_generator.getCustomCharacterSet());
    QCOMPARE(m_generator.getExcludedCharacterSet(), default_generator.getExcludedCharacterSet());
    QCOMPARE(m_generator.getLength(), default_generator.getLength());
}
