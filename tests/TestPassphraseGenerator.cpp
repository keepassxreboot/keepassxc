/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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
#include "config-keepassx-tests.h"
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
    generator.setWordSeparator(" ");
    QVERIFY(generator.isValid());

    QString passphrase;
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toLower());

    generator.setWordCase(PassphraseGenerator::LOWERCASE);
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toLower());

    generator.setWordCase(PassphraseGenerator::UPPERCASE);
    passphrase = generator.generatePassphrase();
    QCOMPARE(passphrase, passphrase.toUpper());

    generator.setWordCase(PassphraseGenerator::TITLECASE);
    passphrase = generator.generatePassphrase();
    QRegularExpression regex("^(?:[A-Z][a-z-]* )*[A-Z][a-z-]*$");
    QVERIFY2(regex.match(passphrase).hasMatch(), qPrintable(passphrase));
}

void TestPassphraseGenerator::testUniqueEntriesInWordlist()
{
    PassphraseGenerator generator;
    // set the limit down, so we don;t have to do a very large file
    generator.m_minimum_wordlist_length = 4;

    // link to bad wordlist
    QString path = QString(KEEPASSX_TEST_DATA_DIR).append("/wordlists/bad_wordlist_with_duplicate_entries.wordlist");

    // setting will work, it creates the warning however, and isValid will fail
    generator.setWordList(path);
    // so this fails
    QVERIFY(!generator.isValid());
}
