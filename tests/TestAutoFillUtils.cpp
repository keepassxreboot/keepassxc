/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "TestAutoFillUtils.h"

#include "autofill/AutoFillUtils.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestAutoFillUtils)

void TestAutoFillUtils::testNormalizeHost_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Simple host") << "example.com" << "example.com";
    QTest::newRow("With trailing dot") << "example.com." << "example.com";
    QTest::newRow("With multiple trailing dots") << "example.com..." << "example.com";
    QTest::newRow("Uppercase") << "EXAMPLE.COM" << "example.com";
    QTest::newRow("Mixed case") << "ExAmPlE.CoM" << "example.com";
    QTest::newRow("Leading spaces") << "  example.com" << "example.com";
    QTest::newRow("Trailing spaces") << "example.com  " << "example.com";
    QTest::newRow("Both spaces and trailing dots") << "  example.com..  " << "example.com";
    QTest::newRow("Empty string") << "" << "";
    QTest::newRow("Only spaces") << "   " << "";
    QTest::newRow("Only dots") << "..." << "";
    QTest::newRow("Subdomain") << "www.example.com." << "www.example.com";
}

void TestAutoFillUtils::testNormalizeHost()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(AutoFillUtils::normalizeHost(input), expected);
}

void TestAutoFillUtils::testHostFromUrl_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    // Valid URLs
    QTest::newRow("Simple HTTPS URL") << "https://example.com" << "example.com";
    QTest::newRow("HTTPS with path") << "https://example.com/path/to/page" << "example.com";
    QTest::newRow("HTTPS with port") << "https://example.com:8080" << "example.com";
    QTest::newRow("HTTPS with query") << "https://example.com?query=1" << "example.com";
    QTest::newRow("HTTP URL") << "http://example.com" << "example.com";
    QTest::newRow("Subdomain") << "https://www.example.com" << "www.example.com";
    QTest::newRow("Deep subdomain") << "https://api.v2.example.com" << "api.v2.example.com";

    // Domain only (no scheme)
    QTest::newRow("Domain without scheme") << "example.com" << "example.com";
    QTest::newRow("Subdomain without scheme") << "www.example.com" << "www.example.com";

    // Special cases
    QTest::newRow("Localhost") << "localhost" << "localhost";
    QTest::newRow("Localhost with scheme") << "http://localhost" << "localhost";
    QTest::newRow("Localhost with port") << "http://localhost:3000" << "localhost";

    // Invalid inputs
    QTest::newRow("Empty string") << "" << "";
    QTest::newRow("Only spaces") << "   " << "";
    QTest::newRow("Invalid characters") << "example<>.com" << "";
    QTest::newRow("Unicode domain") << "example\xC3\xA9.com" << ""; // Contains non-ASCII

    // Edge cases
    QTest::newRow("Trailing dot URL") << "https://example.com." << "example.com";
    QTest::newRow("Uppercase URL") << "HTTPS://EXAMPLE.COM" << "example.com";
}

void TestAutoFillUtils::testHostFromUrl()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(AutoFillUtils::hostFromUrl(input), expected);
}

void TestAutoFillUtils::testHostsMatch_data()
{
    QTest::addColumn<QString>("requested");
    QTest::addColumn<QString>("candidate");
    QTest::addColumn<bool>("expected");

    // Exact matches
    QTest::newRow("Exact match") << "example.com" << "example.com" << true;
    QTest::newRow("Exact match with subdomain") << "www.example.com" << "www.example.com" << true;

    // Subdomain matching
    QTest::newRow("Subdomain of candidate") << "www.example.com" << "example.com" << true;
    QTest::newRow("Candidate is subdomain") << "example.com" << "www.example.com" << true;
    QTest::newRow("Deep subdomain match") << "api.v2.example.com" << "example.com" << true;
    QTest::newRow("Multiple subdomain levels") << "a.b.c.example.com" << "example.com" << true;

    // Non-matches
    QTest::newRow("Different domains") << "example.com" << "other.com" << false;
    QTest::newRow("Similar but different") << "myexample.com" << "example.com" << false;
    QTest::newRow("Suffix match but not subdomain") << "notexample.com" << "example.com" << false;
    QTest::newRow("Partial match") << "example" << "example.com" << false;
    QTest::newRow("TLD mismatch") << "example.org" << "example.com" << false;

    // Empty cases
    QTest::newRow("Empty requested") << "" << "example.com" << false;
    QTest::newRow("Empty candidate") << "example.com" << "" << false;
    QTest::newRow("Both empty") << "" << "" << false;

    // TLD / single-label abuse (must not leak credentials across unrelated domains)
    QTest::newRow("Bare TLD candidate") << "evil.com" << "com" << false;
    QTest::newRow("Bare TLD requested") << "com" << "evil.com" << false;
    QTest::newRow("IP partial overlap") << "1.2.3.4" << "2.3.4" << false;

    // Edge cases
    QTest::newRow("Same single label") << "localhost" << "localhost" << true;
    QTest::newRow("Dot prefix attack") << ".example.com" << "example.com" << false;
}

void TestAutoFillUtils::testHostsMatch()
{
    QFETCH(QString, requested);
    QFETCH(QString, candidate);
    QFETCH(bool, expected);

    QCOMPARE(AutoFillUtils::hostsMatch(requested, candidate), expected);
}

