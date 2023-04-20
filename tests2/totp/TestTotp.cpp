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
#include "core/Entry.h"
#include "core/Totp.h"

TEST_CASE("Testing TOTP secret parsing", "[totp]")
{
    SECTION("Typical OTP URL")
    {
        QString secret =
            "otpauth://totp/"
            "ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
            "SHA1&digits=6&period=30";
        auto settings = Totp::parseSettings(secret);

        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
        REQUIRE_FALSE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::OTPURL);
        REQUIRE(settings->digits == 6u);
        REQUIRE(settings->step == 30u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha1);
    }

    SECTION("OTP URL with non-default hash type")
    {
        QString secret =
            "otpauth://totp/"
            "ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
            "SHA512&digits=6&period=30";

        auto settings = Totp::parseSettings(secret);

        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
        REQUIRE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::OTPURL);
        REQUIRE(settings->digits == 6u);
        REQUIRE(settings->step == 30u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha512);
    }

    SECTION("OTP URL with max TOTP step of 24-hours")
    {
        QString secret =
            "otpauth://totp/"
            "ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
            "SHA512&digits=6&period=90000";

        auto settings = Totp::parseSettings(secret);
        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == QString("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
        REQUIRE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::OTPURL);
        REQUIRE(settings->digits == 6u);
        REQUIRE(settings->step == 86400u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha512);
    }

    SECTION("KeeOTP")
    {
        QString secret = "key=HXDMVJECJJWSRBY%3d&step=25&size=8&otpHashMode=Sha256";

        auto settings = Totp::parseSettings(secret);
        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == QString("HXDMVJECJJWSRBY="));
        REQUIRE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::KEEOTP);
        REQUIRE(settings->digits == 8u);
        REQUIRE(settings->step == 25u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha256);
    }

    SECTION("Semi-colon delineated 'TOTP Settings'")
    {
        QString secret = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";

        auto settings = Totp::parseSettings("30;8", secret);
        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == secret);
        REQUIRE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::LEGACY);
        REQUIRE(settings->digits == 8u);
        REQUIRE(settings->step == 30u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha1);
    }

    SECTION("Bare secret (no 'TOTP Settings' attribute)")
    {
        QString secret = "gezdgnbvgy3tqojqgezdgnbvgy3tqojq";

        auto settings = Totp::parseSettings("", secret);
        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == secret);
        REQUIRE_FALSE(settings->custom);
        REQUIRE(settings->format == Totp::StorageFormat::LEGACY);
        REQUIRE(settings->digits == 6u);
        REQUIRE(settings->step == 30u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha1);
    }

    SECTION("Blank secret")
    {
        // Blank both settings and secret (expected failure)
        REQUIRE(Totp::parseSettings("", "").isNull());

        // Some TOTP settings with blank secret (expected failure)
        REQUIRE(Totp::parseSettings("30;8", "").isNull());
    }
}

TEST_CASE("Testing TOTP code generation", "[totp]")
{
    // Test vectors from RFC 6238, https://tools.ietf.org/html/rfc6238#appendix-B
    auto settings = Totp::createSettings("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ", Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP);

    SECTION("6 digit TOTP (default)")
    {
        REQUIRE(Totp::generateTotp(settings, 1234567890) == QString("005924"));
        REQUIRE(Totp::generateTotp(settings, 1111111109) == QString("081804"));
    }

    SECTION("8 digit TOTP (custom)")
    {
        settings->digits = 8;
        settings->custom = true;

        REQUIRE(Totp::generateTotp(settings, 1111111111) == QString("14050471"));
        REQUIRE(Totp::generateTotp(settings, 2000000000) == QString("69279037"));
    }

    SECTION("Sha256 algorithm (custom)")
    {
        settings->algorithm = Totp::Algorithm::Sha256;

        REQUIRE(Totp::generateTotp(settings, 1234567890) == QString("829826"));
        REQUIRE(Totp::generateTotp(settings, 1111111109) == QString("756375"));
    }

    SECTION("Sha512 algorithm (custom)")
    {
        settings->algorithm = Totp::Algorithm::Sha512;

        REQUIRE(Totp::generateTotp(settings, 1234567890) == QString("671578"));
        REQUIRE(Totp::generateTotp(settings, 1111111109) == QString("049338"));
    }

    SECTION("With time 0 the calculation is based on the current time")
    {
        // It is impossible to match an exact value, just make sure the actual result is 6 digit number.
        REQUIRE_THAT(Totp::generateTotp(settings, 0).toStdString(), Catch::Matchers::Matches("^[0-9]{6}$"));
    }
}

TEST_CASE("Testing TOTP with steam encoder", "[totp]")
{
    SECTION("OTP URL Parsing")
    {
        QString secret = "otpauth://totp/"
                         "test:test@example.com?secret=63BEDWCQZKTQWPESARIERL5DTTQFCJTK&issuer=Valve&algorithm="
                         "SHA1&digits=5&period=30&encoder=steam";
        auto settings = Totp::parseSettings(secret);

        REQUIRE_FALSE(settings.isNull());
        REQUIRE(settings->key == QString("63BEDWCQZKTQWPESARIERL5DTTQFCJTK"));
        REQUIRE_FALSE(settings->custom);
        REQUIRE(settings->encoder.shortName == Totp::STEAM_SHORTNAME);
        REQUIRE(settings->format == Totp::StorageFormat::OTPURL);
        REQUIRE(settings->digits == Totp::STEAM_DIGITS);
        REQUIRE(settings->step == 30u);
        REQUIRE(settings->algorithm == Totp::Algorithm::Sha1);

        // These time/value pairs were created by running the Steam Guard function of the
        // Steam mobile app with a throw-away steam account. The above secret was extracted
        // from the Steam app's data for use in testing here.
        REQUIRE(Totp::generateTotp(settings, 1511200518) == QString("FR8RV"));
        REQUIRE(Totp::generateTotp(settings, 1511200714) == QString("9P3VP"));
    }
}

TEST_CASE("Testing TOTP encoders", "[totp]")
{
    SECTION("Get encoder by name")
    {
        REQUIRE(Totp::getEncoderByName("").alphabet == QString("0123456789"));
        REQUIRE(Totp::getEncoderByName("unknown").alphabet == QString("0123456789"));
        REQUIRE(Totp::getEncoderByName("steam").alphabet == QString("23456789BCDFGHJKMNPQRTVWXY"));
    }

    SECTION("Get encoder by short name")
    {
        REQUIRE(Totp::getEncoderByShortName("").alphabet == QString("0123456789"));
        REQUIRE(Totp::getEncoderByShortName("unknown").alphabet == QString("0123456789"));
        REQUIRE(Totp::getEncoderByShortName("S").alphabet == QString("23456789BCDFGHJKMNPQRTVWXY"));
    }

    SECTION("Get Steam encoder")
    {
        REQUIRE(Totp::steamEncoder().alphabet == QString("23456789BCDFGHJKMNPQRTVWXY"));
    }
}

TEST_CASE("Testing TOTP algorithms", "[totp]")
{
    SECTION("Get supported algorithms")
    {
        auto actual = Totp::supportedAlgorithms();

        REQUIRE(actual.size() == 3);

        REQUIRE(actual[0].first == "SHA-1");
        REQUIRE(actual[0].second == Totp::Algorithm::Sha1);

        REQUIRE(actual[1].first == "SHA-256");
        REQUIRE(actual[1].second == Totp::Algorithm::Sha256);

        REQUIRE(actual[2].first == "SHA-512");
        REQUIRE(actual[2].second == Totp::Algorithm::Sha512);
    }
}

TEST_CASE("Testing TOTP write settings", "[totp]")
{
    SECTION("When the input settings is null")
    {
        auto settings = QSharedPointer<Totp::Settings>(nullptr);
        REQUIRE(Totp::writeSettings(settings, "Title", "UserName").isEmpty());
    }

    SECTION("When the input settings is OTPURL")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::OTPURL);
        REQUIRE(Totp::writeSettings(settings, "Title", "User")
                == "otpauth://totp/Title:User?secret=Key&period=16&digits=6&issuer=Title");
    }

    SECTION("When the input settings is OTPURL with empty title/username")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::OTPURL);
        REQUIRE(Totp::writeSettings(settings, "", "")
                == "otpauth://totp/KeePassXC:none?secret=Key&period=16&digits=6&issuer=KeePassXC");
    }

    SECTION("When the input settings is OTPURL and custom encoder/algorithm")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::OTPURL, "S", Totp::Algorithm::Sha256);
        REQUIRE(
            Totp::writeSettings(settings, "Title", "User")
            == "otpauth://totp/Title:User?secret=Key&period=16&digits=6&issuer=Title&encoder=steam&algorithm=SHA256");
    }

    SECTION("When the input settings is KEEOTP")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::KEEOTP);
        REQUIRE(Totp::writeSettings(settings, "Title", "User") == "key=Key&size=6&step=16");
    }

    SECTION("When the input settings is KEEOTP and custom algorithm")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::KEEOTP, "", Totp::Algorithm::Sha256);
        REQUIRE(Totp::writeSettings(settings, "Title", "User") == "key=Key&size=6&step=16&otpHashMode=SHA256");
    }

    SECTION("When the input settings is LEGACY")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::LEGACY);
        REQUIRE(Totp::writeSettings(settings, "Title", "User") == "16;6");
    }

    SECTION("When the input settings is LEGACY with custom encoder")
    {
        auto settings = Totp::createSettings("Key", 6, 16, Totp::StorageFormat::LEGACY, "S");
        REQUIRE(Totp::writeSettings(settings, "Title", "User") == "16;S");
    }
}

// TODO: Actually this is an Entry related test rather TOTP, move the test under Entry test suite.
TEST_CASE("Testing TOTP in entry history", "[entry]")
{
    GIVEN("An Entry without TOTP and clean history")
    {
        Entry entry;
        REQUIRE_FALSE(entry.hasTotp());
        REQUIRE(entry.historyItems().isEmpty());

        WHEN("Add a new TOTP settings to the entry")
        {
            auto settings = Totp::createSettings("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ", 6, 16);
            entry.setTotp(settings);
            REQUIRE(entry.hasTotp());
            REQUIRE(entry.historyItems().size() == 1);
            REQUIRE(entry.totpSettings()->key == QString("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"));

            AND_WHEN("Set the updated TOTP settings")
            {
                settings->key = "foo";
                entry.setTotp(settings);

                THEN("History and settings were changed")
                {
                    REQUIRE(entry.historyItems().size() == 2);
                    REQUIRE(entry.totpSettings()->key == QString("foo"));
                }
            }

            AND_WHEN("Set the null for TOTP settings")
            {
                entry.setTotp(nullptr);

                THEN("Expected reset of TOTP")
                {
                    REQUIRE_FALSE(entry.hasTotp());
                    REQUIRE(entry.historyItems().size() == 2);
                }
            }
        }
    }
}