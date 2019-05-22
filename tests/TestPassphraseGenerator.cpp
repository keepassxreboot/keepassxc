/*
 *  Copyright (C) 2018-2019 KeePassXC Team <team@keepassxc.org>
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

#include "TestPassphraseGenerator.h"
#include "core/PassphraseGenerator.h"
#include "crypto/Crypto.h"

#include <QRegularExpression>
#include <QTest>

QTEST_GUILESS_MAIN(TestPassphraseGenerator)

void TestPassphraseGenerator::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestPassphraseGenerator::testWordCase()
{
    PassphraseGenerator generator;

    generator.setWordCount(1);
    generator.setWordCase(-3); // invalid - should default to lowercase
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QVERIFY(password.isLower());

    generator.setWordCount(1);
    generator.setWordCase(99); // invalid - should default to lowercase
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QVERIFY(password.isLower());

    generator.setWordCount(1);
    generator.setWordCase(PassphraseGenerator::LOWERCASE);
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QVERIFY(password.isLower());

    generator.setWordCount(1);
    generator.setWordCase(PassphraseGenerator::UPPERCASE);
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QVERIFY(password.isUpper());

    generator.setWordCount(1);
    generator.setWordCase(PassphraseGenerator::TITLECASE);
    QVERIFY(generator.isValid());
    QString password = generator.generatePassword();
    QRegularExpression regex("^[A-Z][a-z]*+$");
    QVERIFY(regex.match(password).hasMatch());
}
