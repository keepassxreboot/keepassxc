#include "TestIconDownloader.h"
#include <QTest>
#include <gui/IconDownloader.h>

QTEST_GUILESS_MAIN(TestIconDownloader)

void TestIconDownloader::testIconDownloader()
{
    QFETCH(QString, url);
    QFETCH(QStringList, expectation);

    IconDownloader downloader;
    downloader.setUrl(url);
    QStringList resolved_urls;
    for (const auto& resolved_url : downloader.m_urlsToTry) {
        resolved_urls.push_back(resolved_url.toString());
    }

    QCOMPARE(resolved_urls, expectation);
}

void TestIconDownloader::testIconDownloader_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QStringList>("expectation");

    const QString keepassxc_favicon("https://keepassxc.org/favicon.ico");

    QTest::newRow("Invalid URL") << "http:sk/s.com" << QStringList{};
    QTest::newRow("Unsupported schema") << "ftp://google.com" << QStringList{};
    QTest::newRow("Missing schema") << "keepassxc.org" << QStringList{"https://keepassxc.org/favicon.ico"};
    QTest::newRow("Missing host") << "https:///register" << QStringList{};
    QTest::newRow("URL with path") << "https://keepassxc.org/register/here/"
                                   << QStringList{"https://keepassxc.org/register/here/favicon.ico", keepassxc_favicon};
    QTest::newRow("URL with path and query")
        << "https://keepassxc.org/register/here?login=me"
        << QStringList{"https://keepassxc.org/register/favicon.ico", keepassxc_favicon};
    QTest::newRow("URL with port") << "https://keepassxc.org:8080"
                                   << QStringList{"https://keepassxc.org:8080/favicon.ico"};
    QTest::newRow("2nd level domain") << "https://login.keepassxc.org"
                                      << QStringList{"https://login.keepassxc.org/favicon.ico", keepassxc_favicon};
    QTest::newRow("2nd level domain with additional fields")
        << "https://user:password@login.keepassxc.org:2021?test#1"
        << QStringList{"https://user:password@login.keepassxc.org:2021/favicon.ico",
                       "https://user:password@keepassxc.org:2021/favicon.ico",
                       keepassxc_favicon};
    QTest::newRow("2nd level domain (.co.uk special case), with subdomain")
        << "https://login.keepassxc.co.uk"
        << QStringList{"https://login.keepassxc.co.uk/favicon.ico", "https://keepassxc.co.uk/favicon.ico"};
    QTest::newRow("2nd level domain .co.uk special case")
        << "https://keepassxc.co.uk" << QStringList{"https://keepassxc.co.uk/favicon.ico"};
    QTest::newRow("2nd level domain with several subdomains")
        << "https://de.login.keepassxc.org"
        << QStringList{"https://de.login.keepassxc.org/favicon.ico", keepassxc_favicon};
    QTest::newRow("Raw IP with schema") << "https://134.130.155.184"
                                        << QStringList{"https://134.130.155.184/favicon.ico"};
    QTest::newRow("Raw IP") << "134.130.155.184" << QStringList{"https://134.130.155.184/favicon.ico"};
    QTest::newRow("Raw IP with schema and path")
        << "https://134.130.155.184/with/path/"
        << QStringList{"https://134.130.155.184/with/path/favicon.ico", "https://134.130.155.184/favicon.ico"};
    QTest::newRow("Raw IP with schema (https), path, and port")
        << "https://134.130.155.184:8080/test/"
        << QStringList{"https://134.130.155.184:8080/test/favicon.ico", "https://134.130.155.184:8080/favicon.ico"};
    QTest::newRow("Raw IP with schema (http), path, and port")
        << "134.130.155.184:8080/test/"
        << QStringList{"https://134.130.155.184:8080/test/favicon.ico", "https://134.130.155.184:8080/favicon.ico"};
    QTest::newRow("URL with username and password")
        << "https://user:password@keepassxc.org" << QStringList{"https://user:password@keepassxc.org/favicon.ico"};
    QTest::newRow("URL with username and password, several subdomains")
        << "https://user:password@de.login.keepassxc.org"
        << QStringList{"https://user:password@de.login.keepassxc.org/favicon.ico",
                       "https://user:password@keepassxc.org/favicon.ico",
                       "https://keepassxc.org/favicon.ico"};
    QTest::newRow("Raw IP with username and password")
        << "https://user:password@134.130.155.184" << QStringList{"https://user:password@134.130.155.184/favicon.ico"};
    QTest::newRow("Relative path should be preserved")
        << "https://test.com/rel-path/"
        << QStringList{"https://test.com/rel-path/favicon.ico", "https://test.com/favicon.ico"};
}
