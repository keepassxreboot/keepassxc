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

#include "TestUrlTools.h"
#include <QTest>

QTEST_GUILESS_MAIN(TestUrlTools)

void TestUrlTools::initTestCase()
{
    m_urlTools = urlTools();
}

void TestUrlTools::init()
{
}

void TestUrlTools::testTopLevelDomain()
{
    // Create list of URLs and expected TLD responses
    QList<QPair<QString, QString>> tldUrls{
        {QLatin1String("https://another.example.co.uk"), QLatin1String("co.uk")},
        {QLatin1String("https://www.example.com"), QLatin1String("com")},
        {QLatin1String("https://example.com"), QLatin1String("com")},
        {QLatin1String("https://github.com"), QLatin1String("com")},
        {QLatin1String("http://test.net"), QLatin1String("net")},
        {QLatin1String("http://so.many.subdomains.co.jp"), QLatin1String("co.jp")},
        {QLatin1String("https://192.168.0.1"), QLatin1String("192.168.0.1")},
        {QLatin1String("https://192.168.0.1:8000"), QLatin1String("192.168.0.1")},
        {QLatin1String("https://www.nic.ar"), QLatin1String("ar")},
        {QLatin1String("https://no.no.no"), QLatin1String("no")},
        {QLatin1String("https://www.blogspot.com.ar"), QLatin1String("blogspot.com.ar")}, // blogspot.com.ar is a TLD
        {QLatin1String("https://jap.an.ide.kyoto.jp"), QLatin1String("ide.kyoto.jp")}, // ide.kyoto.jp is a TLD
        {QLatin1String("ar"), QLatin1String("ar")},
    };

    for (const auto& u : tldUrls) {
        QCOMPARE(urlTools()->getTopLevelDomainFromUrl(u.first), u.second);
    }

    // Create list of URLs and expected base URL responses
    QList<QPair<QString, QString>> baseUrls{
        {QLatin1String("https://another.example.co.uk"), QLatin1String("example.co.uk")},
        {QLatin1String("https://www.example.com"), QLatin1String("example.com")},
        {QLatin1String("http://test.net"), QLatin1String("test.net")},
        {QLatin1String("http://so.many.subdomains.co.jp"), QLatin1String("subdomains.co.jp")},
        {QLatin1String("https://192.168.0.1"), QLatin1String("192.168.0.1")},
        {QLatin1String("https://192.168.0.1:8000"), QLatin1String("192.168.0.1")},
        {QLatin1String("https://www.nic.ar"), QLatin1String("nic.ar")},
        {QLatin1String("https://www.blogspot.com.ar"), QLatin1String("www.blogspot.com.ar")}, // blogspot.com.ar is a TLD
        {QLatin1String("https://www.arpa"), QLatin1String("www.arpa")},
        {QLatin1String("https://jap.an.ide.kyoto.jp"), QLatin1String("an.ide.kyoto.jp")}, // ide.kyoto.jp is a TLD
        {QLatin1String("https://kobe.jp"), QLatin1String("kobe.jp")},
    };

    for (const auto& u : baseUrls) {
        QCOMPARE(urlTools()->getBaseDomainFromUrl(u.first), u.second);
    }
}

void TestUrlTools::testIsIpAddress()
{
    auto host1 = "example.com"; // Not valid
    auto host2 = "192.168.0.1";
    auto host3 = "278.21.2.0"; // Not valid
    auto host4 = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
    auto host5 = "2001:db8:0:1:1:1:1:1";
    auto host6 = "fe80::1ff:fe23:4567:890a";
    auto host7 = "2001:20::1";
    auto host8 = "2001:0db8:85y3:0000:0000:8a2e:0370:7334"; // Not valid
    auto host9 = "[::]";
    auto host10 = "::";
    auto host11 = "[2001:20::1]";

    QVERIFY(!urlTools()->isIpAddress(host1));
    QVERIFY(urlTools()->isIpAddress(host2));
    QVERIFY(!urlTools()->isIpAddress(host3));
    QVERIFY(urlTools()->isIpAddress(host4));
    QVERIFY(urlTools()->isIpAddress(host5));
    QVERIFY(urlTools()->isIpAddress(host6));
    QVERIFY(urlTools()->isIpAddress(host7));
    QVERIFY(!urlTools()->isIpAddress(host8));
    QVERIFY(urlTools()->isIpAddress(host9));
    QVERIFY(urlTools()->isIpAddress(host10));
    QVERIFY(urlTools()->isIpAddress(host11));
}

void TestUrlTools::testIsUrlIdentical()
{
    QVERIFY(urlTools()->isUrlIdentical("https://example.com", "https://example.com"));
    QVERIFY(urlTools()->isUrlIdentical("https://example.com", "  https://example.com  "));
    QVERIFY(!urlTools()->isUrlIdentical("https://example.com", "https://example2.com"));
    QVERIFY(!urlTools()->isUrlIdentical("https://example.com/", "https://example.com/#login"));
    QVERIFY(urlTools()->isUrlIdentical("https://example.com", "https://example.com/"));
    QVERIFY(urlTools()->isUrlIdentical("https://example.com/", "https://example.com"));
    QVERIFY(urlTools()->isUrlIdentical("https://example.com/  ", "  https://example.com"));
    QVERIFY(!urlTools()->isUrlIdentical("https://example.com/", "  example.com"));
    QVERIFY(urlTools()->isUrlIdentical("https://example.com/path/to/nowhere", "https://example.com/path/to/nowhere/"));
    QVERIFY(!urlTools()->isUrlIdentical("https://example.com/", "://example.com/"));
    QVERIFY(urlTools()->isUrlIdentical("ftp://127.0.0.1/", "ftp://127.0.0.1"));
}

void TestUrlTools::testIsUrlValid()
{
    QHash<QString, bool> urls;
    urls["https://github.com/login"] = true;
    urls["https:///github.com/"] = false;
    urls["http://github.com/**//*"] = false;
    urls["http://*.github.com/login"] = false;
    urls["//github.com"] = true;
    urls["github.com/{}<>"] = false;
    urls["http:/example.com"] = false;
    urls["http:/example.com."] = false;
    urls["cmd://C:/Toolchains/msys2/usr/bin/mintty \"ssh jon@192.168.0.1:22\""] = true;
    urls["file:///Users/testUser/Code/test.html"] = true;
    urls["{REF:A@I:46C9B1FFBD4ABC4BBB260C6190BAD20C} "] = true;

    QHashIterator<QString, bool> i(urls);
    while (i.hasNext()) {
        i.next();
        QCOMPARE(urlTools()->isUrlValid(i.key()), i.value());
    }
}

void TestUrlTools::testDomainHasIllegalCharacters()
{
    QVERIFY(!urlTools()->domainHasIllegalCharacters("example.com"));
    QVERIFY(urlTools()->domainHasIllegalCharacters("domain has spaces.com"));
    QVERIFY(urlTools()->domainHasIllegalCharacters("example#|.com"));
}
