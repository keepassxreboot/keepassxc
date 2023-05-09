#include "TestNetworkRequest.h"
#include "core/NetworkRequest.h"
#include "mock/MockNetworkAccessManager.h"
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

QTEST_GUILESS_MAIN(TestNetworkRequest)

void TestNetworkRequest::testNetworkRequest()
{
    QFETCH(QUrl, requestedURL);
    QFETCH(QByteArray, expectedContent);
    QFETCH(QString, expectedUserAgent);
    QFETCH(bool, expectError);
    QFETCH(QNetworkReply::NetworkError, error);

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    auto& reply = manager
        .whenGet(requestedURL)
        // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();
    if (!expectError) {
        reply.withBody(expectedContent);
    } else {
        reply.withError(error);
    }

    // Create request
    NetworkRequest request = createRequest(5, std::chrono::milliseconds{5000}, QList<QPair<QString, QString>>{}, &manager);
    request.fetch(requestedURL);

    QString actualContent;
    bool didError = false, didSucceed = false;

    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&actualContent, &didSucceed](QByteArray content) {
        actualContent = QString(content);
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });


    QTest::qWait(3*100);

    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(manager.matchedRequests().length(), 1);
    if(!expectError) {
        // Ensures that NetworkRequest parses the reply properly
        // URL correct?
        QCOMPARE(request.url(), requestedURL);
        // Content correct?
        QCOMPARE(actualContent, expectedContent);
        QCOMPARE(didSucceed, true);
        QCOMPARE(didError, false);
    } else {
        // Ensures that NetworkRequest parses the reply properly
        // URL correct?
        QCOMPARE(request.url(), requestedURL);
        // Content correct?
        QCOMPARE(didSucceed, false);
        QCOMPARE(didError, true);
    }
}
void TestNetworkRequest::testNetworkRequest_data()
{
    QTest::addColumn<QUrl>("requestedURL");
    QTest::addColumn<QByteArray>("expectedContent");
    QTest::addColumn<QString>("expectedUserAgent");
    QTest::addColumn<bool>("expectError");
    QTest::addColumn<QNetworkReply::NetworkError>("error");

    QString defaultUserAgent("KeePassXC");

    const QUrl& exampleURL = QUrl{"https://example.com"};
    const QByteArray& exampleContent = QString{"test-content"}.toUtf8();

    QTest::newRow("successful request") << exampleURL << exampleContent << defaultUserAgent
        << false << QNetworkReply::NetworkError::NoError;
}

void TestNetworkRequest::testNetworkRequestTimeout()
{
    // TODO
}
void TestNetworkRequest::testNetworkRequestRedirects()
{
    // TODO
}
