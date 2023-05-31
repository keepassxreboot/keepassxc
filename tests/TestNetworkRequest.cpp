#include "TestNetworkRequest.h"
#include "core/NetworkRequest.h"
#include "mock/MockNetworkAccessManager.h"
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

QTEST_GUILESS_MAIN(TestNetworkRequest)

using ContentTypeParameters_t = QHash<QString, QString>;
Q_DECLARE_METATYPE(ContentTypeParameters_t);
Q_DECLARE_METATYPE(std::chrono::milliseconds);

namespace {
    void processRequests() {
        QTest::qWait(300);
    }
}

void TestNetworkRequest::testNetworkRequest()
{
    QFETCH(QUrl, requestedURL);
    QFETCH(QByteArray, expectedContent);
    QFETCH(QString, responseContentType);
    QFETCH(QString, expectedContentType);
    QFETCH(ContentTypeParameters_t, expectedContentTypeParameters);
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
        reply.withBody(expectedContent).withHeader(QNetworkRequest::ContentTypeHeader, responseContentType);
    } else {
        reply.withError(error);
    }

    // Create request
    NetworkRequest request = NetworkRequestBuilder(requestedURL).setManager(&manager).build();

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

    request.fetch();
    processRequests();

    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(manager.matchedRequests().length(), 1);
    QCOMPARE(request.URL(), requestedURL);
    if(!expectError) {
        QCOMPARE(actualContent, expectedContent);
        QCOMPARE(request.ContentType(), expectedContentType);
        QCOMPARE(request.ContentTypeParameters(), expectedContentTypeParameters);
        QCOMPARE(didSucceed, true);
        QCOMPARE(didError, false);
        QCOMPARE(request.Reply()->isFinished(), true);
    } else {
        QCOMPARE(didSucceed, false);
        QCOMPARE(didError, true);
    }
}
void TestNetworkRequest::testNetworkRequest_data()
{
    QTest::addColumn<QUrl>("requestedURL");
    QTest::addColumn<QByteArray>("expectedContent");
    QTest::addColumn<QString>("responseContentType");
    QTest::addColumn<QString>("expectedContentType");
    QTest::addColumn<ContentTypeParameters_t>("expectedContentTypeParameters");
    QTest::addColumn<QString>("expectedUserAgent");
    QTest::addColumn<bool>("expectError");
    QTest::addColumn<QNetworkReply::NetworkError>("error");

    QString defaultUserAgent("KeePassXC");

    const QUrl& exampleURL = QUrl{"https://example.com"};
    const QByteArray& exampleContent = QString{"test-content"}.toUtf8();

    QTest::newRow("successful request") << exampleURL << exampleContent << "text/plain"
                                        << "text/plain" << ContentTypeParameters_t{}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type") << exampleURL << exampleContent << "application/test-content-type"
                                        << "application/test-content-type" << ContentTypeParameters_t{}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("empty content type") << exampleURL << QByteArray{} << "" << "" << ContentTypeParameters_t{}
                                  << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type parameters") << exampleURL << exampleContent << "application/test-content-type;test-param=test-value"
                                        << "application/test-content-type" << ContentTypeParameters_t {{"test-param", "test-value"}}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type parameters trimmed") << exampleURL << exampleContent << "application/test-content-type; test-param = test-value"
                                        << "application/test-content-type" << ContentTypeParameters_t {{"test-param", "test-value"}}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
}

void TestNetworkRequest::testNetworkRequestTimeout()
{
    // Timeout should work for single request
    // Timeout should capture entire duration, including redirects
    QFETCH(bool, expectError);
    QFETCH(std::chrono::milliseconds, delay);
    QFETCH(std::chrono::milliseconds, timeout);

    const auto requestedURL = QUrl("https://example.com");
    const auto expectedUserAgent = QString("KeePassXC");

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    auto& reply = manager
        .whenGet(requestedURL)
            // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();

    // Timeout
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(delay);

    reply.withFinishDelayUntil(&timer, &QTimer::timeout);

    // Create request
    NetworkRequest request = NetworkRequestBuilder(requestedURL).setManager(&manager).setTimeout(timeout).build();

    // Start timer
    timer.start();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    processRequests();

    QTEST_ASSERT(didError || didSucceed);

    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(manager.matchedRequests().length(), 1);
    QCOMPARE(request.URL(), requestedURL);
    QCOMPARE(didSucceed, !expectError);
    QCOMPARE(didError, expectError);
}

void TestNetworkRequest::testNetworkRequestTimeout_data()
{

    QTest::addColumn<bool>("expectError");
    QTest::addColumn<std::chrono::milliseconds>("delay");
    QTest::addColumn<std::chrono::milliseconds>("timeout");

    QTest::newRow("timeout") << true << std::chrono::milliseconds{100} << std::chrono::milliseconds{50};
    QTest::newRow("no timeout") << false << std::chrono::milliseconds{50} << std::chrono::milliseconds{100};
}

void TestNetworkRequest::testNetworkRequestRedirects()
{
    // Should respect max number of redirects
    // Headers, Reply, etc. should reflect final request

    QFETCH(int, numRedirects);
    QFETCH(int, maxRedirects);
    const bool expectError = numRedirects > maxRedirects;

    const auto requestedURL = QUrl("https://example.com");
    const auto expectedUserAgent = QString("KeePassXC");

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    QStringList requestedUrls;

    auto* reply = &manager
        .whenGet(requestedURL)
            // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();

    for(int i = 0; i < numRedirects; ++i) {
        auto redirectTarget = QUrl("https://example.com/redirect" + QString::number(i));
        reply->withRedirect(redirectTarget);
        reply = &manager.whenGet(redirectTarget)
            // Has right user agent?
            .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                               QRegularExpression(expectedUserAgent)))
            .reply();
    }
    reply->withBody(QString{"test-content"}.toUtf8());

    // Create request
    NetworkRequest request = NetworkRequestBuilder(requestedURL).setManager(&manager)
                                 .setMaxRedirects(maxRedirects).build();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    processRequests();

    QTEST_ASSERT(didError || didSucceed);
    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(didSucceed, !expectError);
    QCOMPARE(didError, expectError);
    if(didSucceed) {
        QCOMPARE(manager.matchedRequests().length(), numRedirects + 1);
        QCOMPARE(request.URL(), requestedURL);
    }
}

void TestNetworkRequest::testNetworkRequestRedirects_data()
{
    QTest::addColumn<int>("numRedirects");
    QTest::addColumn<int>("maxRedirects");

    QTest::newRow("fewer redirects than allowed (0)") << 0 << 5;
    QTest::newRow("fewer redirects than allowed (1)") << 1 << 5;
    QTest::newRow("fewer redirects than allowed (2)") << 2 << 5;
    QTest::newRow("more redirects than allowed (1, 0)") << 1 << 0;
    QTest::newRow("more redirects than allowed (2, 1)") << 2 << 1;
    QTest::newRow("more redirects than allowed (3, 2)") << 3 << 2;
}