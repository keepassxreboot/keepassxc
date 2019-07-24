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

void TestPassphraseGenerator::testWordCaseOptions()
{
    PassphraseGenerator generator;
    QString passphrase;
    
    // Basic test to confirm isValid
    QVERIFY(!generator.isValid());
    generator.setDefaultWordList();
    generator.setWordCount(1);
    QVERIFY(generator.isValid());

    // Confirm we at least get something
    QVERIFY(generator.generatePassphrase().size() > 0);

    // By default we expect a lowercase passphrase
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toLower());

    // Test upper case
    generator.setWordCase(PassphraseGenerator::WordCaseOption::Upper);
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toUpper());
    
    // Test capital case - in this case we test for mixed case
    generator.setWordCase(PassphraseGenerator::WordCaseOption::Capital);
    passphrase = generator.generatePassphrase();
    QVERIFY(!(passphrase == passphrase.toUpper()));
    QVERIFY(!(passphrase == passphrase.toLower()));

    // Test explicit lower case
    generator.setWordCase(PassphraseGenerator::WordCaseOption::Lower);
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toLower());
}
