
#include "NetworkRequest.h"
#include "NetworkManager.h"
#include <QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {
    QList<QPair<QString, QString>> createDefaultHeaders() {
        QList<QPair<QString, QString>> headers;
        headers.append(QPair{"User-Agent", "KeePassXC"});
        return headers;
    }
}

static constexpr int DEFAULT_MAX_REDIRECTS = 5;
static constexpr std::chrono::milliseconds DEFAULT_TIMEOUT = std::chrono::seconds(5);
static QList<QPair<QString, QString>> DEFAULT_HEADERS = createDefaultHeaders();

namespace
{
    QUrl getRedirectTarget(QNetworkReply* reply)
    {
        QVariant var = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        QUrl url;
        if (var.canConvert<QUrl>()) {
            url = var.toUrl();
        }
        return url;
    }

    QPair<QString, QHash<QString, QString>> parseContentTypeHeader(const QString& contentTypeHeader)
    {
        QString contentType;
        QHash<QString, QString> contentTypeParameters;
        // Parse content type
        auto tokens = contentTypeHeader.split(";", Qt::SkipEmptyParts);
        if(tokens.isEmpty()) {
            return {contentType, contentTypeParameters};
        }
        contentType = tokens[0].trimmed();
        for (int i = 1; i < tokens.size(); ++i) {
            auto parameterTokens = tokens[i].split("=");
            contentTypeParameters[parameterTokens[0].trimmed()] = parameterTokens[1].trimmed();
        }
        return {contentType, contentTypeParameters};
    }
} // namespace

void NetworkRequest::fetch(const QUrl& url)
{
    m_finished = false;

    if(url.scheme() == "http") {
        if(!m_allowInsecure) {
            fail();
            emit failure();
            return;
        }
    } else if (url.scheme() != "https") {
        fail();
        emit failure();
        return;
    }

    QNetworkRequest request(url);

    QSslConfiguration sslConfiguration = request.sslConfiguration();
    sslConfiguration.setProtocol(QSsl::SecureProtocols);
    request.setSslConfiguration(sslConfiguration);

    // Set headers
    for (const auto& [header, value] : qAsConst(m_headers)) {
        request.setRawHeader(header.toUtf8(), value.toUtf8());
    }

    m_reply = m_manager->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &NetworkRequest::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &NetworkRequest::fetchReadyRead);
}

void NetworkRequest::fetchFinished()
{
    if(m_finished)
        return;
    m_finished = true;

    auto error = m_reply->error();
    QUrl redirectTarget = getRedirectTarget(m_reply);
    QUrl url = m_reply->url();

    if (error != QNetworkReply::NoError) {
        // Do not emit on abort.
        if (error != QNetworkReply::OperationCanceledError) {
            fail();
        }
        return;
    }

    if (redirectTarget.isValid()) {
        // Redirected, we need to follow it, or fall through if we have
        // done too many redirects already.
        if (m_redirects < m_maxRedirects) {
            if (redirectTarget.isRelative()) {
                redirectTarget = url.resolved(redirectTarget);
            }
            // Request the redirect target
            m_redirects += 1;
            fetch(redirectTarget);
            return;
        } else {
            fail();
            return;
        }
    }

    // Returns an empty string if the header was not set
    QString contentTypeHeader = m_reply->header(QNetworkRequest::ContentTypeHeader).toString();
    // Parse the header and cache the result
    auto [contentType, contentTypeParameters] = parseContentTypeHeader(contentTypeHeader);
    m_content_type = contentType;
    m_content_type_parameters = contentTypeParameters;

    emit success(std::move(m_bytes));
}

void NetworkRequest::fetchReadyRead()
{
    m_bytes += m_reply->readAll();
}

void NetworkRequest::reset()
{
    m_bytes.clear();
    m_content_type = "";
    m_content_type_parameters.clear();

    // Ensure that replies of redirects are deleted
    if(m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void NetworkRequest::cancel()
{
    if(m_finished)
        return;
    m_finished = true;
    if (m_reply) {
        m_reply->abort();
        // Clear all data and free any resources
        reset();
    }
}

NetworkRequest::~NetworkRequest()
{
    cancel();
}

QUrl NetworkRequest::URL() const
{
    return m_requested_url;
}

NetworkRequest::NetworkRequest(
    QUrl targetURL, bool allowInsecure, unsigned int maxRedirects, std::chrono::milliseconds timeoutDuration, QList<QPair<QString, QString>> headers,
    QNetworkAccessManager* manager)
    : m_manager(manager)
    , m_reply(nullptr)
    , m_finished(false)
    , m_maxRedirects(maxRedirects)
    , m_redirects(0)
    , m_headers(headers)
    , m_requested_url(targetURL)
, m_allowInsecure(allowInsecure)
{
    connect(&m_timeout, &QTimer::timeout, this, &NetworkRequest::fetchTimeout);

    m_timeout.setInterval(timeoutDuration);
    m_timeout.setSingleShot(true);
}

const QString& NetworkRequest::ContentType() const
{
    return m_content_type;
}

const QHash<QString, QString>& NetworkRequest::ContentTypeParameters() const
{
    return m_content_type_parameters;
}

void NetworkRequest::fetchTimeout()
{
    if(m_finished)
        return;
    m_finished = true;
    fail();
}

QNetworkReply* NetworkRequest::Reply() const
{
    return m_reply;
}

void NetworkRequest::fetch()
{
    m_timeout.start();
    fetch(m_requested_url);
}

void NetworkRequest::fail()
{
    cancel();
    emit failure();
}

NetworkRequestBuilder::NetworkRequestBuilder()
{
    this->setAllowInsecure(false);
    this->setMaxRedirects(DEFAULT_MAX_REDIRECTS);
    this->setTimeout(DEFAULT_TIMEOUT);
    this->setHeaders(DEFAULT_HEADERS);
    this->setManager(nullptr);
}

NetworkRequestBuilder::NetworkRequestBuilder(QUrl url) : NetworkRequestBuilder()
{
    this->setUrl(url);
}

NetworkRequestBuilder& NetworkRequestBuilder::setManager(QNetworkAccessManager* manager)
{
    m_manager = manager ? manager : getNetMgr();
    return *this;
}

NetworkRequest NetworkRequestBuilder::build()
{
    return NetworkRequest(m_url, m_allowInsecure, m_maxRedirects, m_timeoutDuration, m_headers, m_manager);
}

NetworkRequestBuilder& NetworkRequestBuilder::setHeaders(QList<QPair<QString, QString>> headers)
{
    m_headers = headers;

    // Append user agent unless given
    if (std::none_of(m_headers.begin(), m_headers.end(), [](const auto& pair) {
        return pair.first == "User-Agent";
    })) {
        m_headers.append(QPair{"User-Agent", "KeePassXC"});
    }

    return *this;
}

NetworkRequestBuilder& NetworkRequestBuilder::setTimeout(std::chrono::milliseconds timeoutDuration)
{
    m_timeoutDuration = timeoutDuration;
    return *this;
}

NetworkRequestBuilder& NetworkRequestBuilder::setUrl(QUrl url)
{
    m_url = url;
    // Default scheme to https.
    if(m_url.scheme().isEmpty()) {
        // Necessary to ensure that setScheme adds // (QUrl needs a valid authority section)
        // For example, QUrl("example.com").setScheme("https") would result in "https:example.com" (invalid
        m_url = QUrl::fromUserInput(m_url.toString(QUrl::FullyEncoded));
        m_url.setScheme("https");
    }
    return *this;
}

NetworkRequestBuilder& NetworkRequestBuilder::setAllowInsecure(bool allowInsecure)
{
    m_allowInsecure = allowInsecure;
    return *this;
}

NetworkRequestBuilder& NetworkRequestBuilder::setMaxRedirects(unsigned int maxRedirects)
{
    m_maxRedirects = maxRedirects;
    return *this;
}
