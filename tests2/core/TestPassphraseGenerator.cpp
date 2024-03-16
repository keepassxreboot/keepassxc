/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#include "../streams.h"
#include "catch2/catch_all.hpp"
#include "config-keepassx-tests.h"
#include "core/PassphraseGenerator.h"

#include <QFile>
#include <QRegularExpression>

TEST_CASE("PassphraseGenerator functionality", "[core]")
{
    PassphraseGenerator generator;
    REQUIRE(generator.isValid());

    SECTION("Default parameters should produce lower case multi-word passphrase")
    {
        QString passphrase = generator.generatePassphrase();
        REQUIRE(passphrase == passphrase.toLower());
        REQUIRE_THAT(passphrase.toStdString(), Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
    }

    SECTION("A passphrase with custom parameters should be as expected")
    {
        generator.setWordCount(3);
        generator.setWordSeparator("|");
        generator.setWordCase(PassphraseGenerator::LOWERCASE);

        QString passphrase = generator.generatePassphrase();
        REQUIRE(passphrase == passphrase.toLower());
        REQUIRE_THAT(passphrase.toStdString(), Catch::Matchers::Matches(R"(^\S+|\S+|\S+$)"));
    }

    SECTION("A passphrase should be in uppercase when WordCase is defined as UPPERCASE")
    {
        generator.setWordCase(PassphraseGenerator::UPPERCASE);

        QString passphrase = generator.generatePassphrase();
        REQUIRE(passphrase == passphrase.toUpper());
    }

    SECTION("A passphrase should be in title-case when WordCase is defined as TITLECASE")
    {
        generator.setWordCase(PassphraseGenerator::TITLECASE);

        QString passphrase = generator.generatePassphrase();
        // Using a dash because the test wordlist has few words like, drop-in, felt-tip, t-shirt, etc.
        REQUIRE_THAT(passphrase.toStdString(), Catch::Matchers::Matches(R"(^([A-Z][a-z,-]*\s?)+$)"));
    }

    SECTION("estimateEntropy with default word list should produce non zero result")
    {
        REQUIRE_THAT(generator.estimateEntropy(0.0), Catch::Matchers::WithinAbs(90.4736875252, 0.0001));
        REQUIRE_THAT(generator.estimateEntropy(1.0), Catch::Matchers::WithinAbs(12.9248125036, 0.0001));
        REQUIRE_THAT(generator.estimateEntropy(10.0), Catch::Matchers::WithinAbs(129.2481250361, 0.0001));
    }

    SECTION("estimateEntropy with empty word list should produce zero result")
    {
        // Reset the word list by trying to load a wrong file.
        generator.setWordList("wrong path");
        REQUIRE(generator.estimateEntropy(0.0) == 0.0);
    }

    SECTION("A passphrase should produce lower case multi-word passphrase when reads a PGP signed wordlist")
    {
        // See full version: https://github.com/ulif/diceware/blob/master/diceware/wordlists/wordlist_en_securedrop.asc
        QString fileName = QString(KEEPASSX_TEST_DATA_DIR).append("/wordlist_pgp_short.asc");
        REQUIRE(QFile::exists(fileName));

        generator.setWordList(fileName);
        REQUIRE(generator.isValid());

        QString passphrase = generator.generatePassphrase();
        REQUIRE(passphrase == passphrase.toLower());
        REQUIRE_THAT(passphrase.toStdString(), Catch::Matchers::Matches(R"(^\S+ \S+ \S+ \S+ \S+ \S+ \S+$)"));
    }
}