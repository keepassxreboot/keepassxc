#include "TestNetworkRequest.h"
#include "core/NetworkRequest.h"
#include "mock/MockNetworkAccessManager.h"
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

QTEST_GUILESS_MAIN(TestNetworkRequest)

using ContentTypeParameters_t = QHash<QString, QString>;
Q_DECLARE_METATYPE(ContentTypeParameters_t);

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
    NetworkRequest request = createRequest(requestedURL, 5, std::chrono::milliseconds{5000}, QList<QPair<QString, QString>>{}, &manager);

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
    // TODO
}
void TestNetworkRequest::testNetworkRequestRedirects()
{
    // Should respect max number of redirects
    // Headers, Reply, etc. should reflect final request
    // TODO
}
