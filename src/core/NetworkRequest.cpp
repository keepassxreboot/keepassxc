
#include "NetworkRequest.h"
#include "NetworkManager.h"
#include <QString>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

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

    QNetworkRequest request(url);

    // Set headers
    for (const auto& [header, value] : qAsConst(m_headers)) {
        request.setRawHeader(header.toUtf8(), value.toUtf8());
    }

    m_reply = m_manager->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &NetworkRequest::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &NetworkRequest::fetchReadyRead);

    m_timeout.start();
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
            emit failure();
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
            qDebug() << "Following redirect to" << redirectTarget;
            m_redirects += 1;
            fetch(redirectTarget);
            return;
        } else {
            qDebug() << "Too many redirects";
            emit failure();
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

void NetworkRequest::setMaxRedirects(int maxRedirects)
{
    m_maxRedirects = std::max(0, maxRedirects);
}

NetworkRequest::NetworkRequest(
    QUrl targetURL,
                                int maxRedirects,
                               std::chrono::milliseconds timeoutDuration,
                               QList<QPair<QString, QString>> headers,
                               QNetworkAccessManager* manager)
    : m_reply(nullptr)
    , m_finished(false)
    , m_maxRedirects(maxRedirects)
    , m_redirects(0)
    , m_timeoutDuration(timeoutDuration)
    , m_headers(headers)
    , m_requested_url(targetURL)
{
    m_manager = manager ? manager : getNetMgr();
    connect(&m_timeout, &QTimer::timeout, this, &NetworkRequest::fetchTimeout);

    m_timeout.setInterval(m_timeoutDuration);
    m_timeout.setSingleShot(true);

    fetch(m_requested_url);
}

const QString& NetworkRequest::ContentType() const
{
    return m_content_type;
}

const QHash<QString, QString>& NetworkRequest::ContentTypeParameters() const
{
    return m_content_type_parameters;
}

void NetworkRequest::setTimeout(std::chrono::milliseconds timeoutDuration)
{
    m_timeoutDuration = timeoutDuration;
}

void NetworkRequest::fetchTimeout()
{
    if(m_finished)
        return;
    m_finished = true;
    // Cancel request on timeout
    cancel();
    emit failure();
}

QNetworkReply* NetworkRequest::Reply() const
{
    return m_reply;
}

NetworkRequest createRequest(QUrl target,int maxRedirects,
                             std::chrono::milliseconds timeoutDuration,
                             QList<QPair<QString, QString>> additionalHeaders,
                             QNetworkAccessManager* manager)
{
    // Append user agent unless given
    if (std::none_of(additionalHeaders.begin(), additionalHeaders.end(), [](const auto& pair) {
            return pair.first == "User-Agent";
        })) {
        additionalHeaders.append(QPair{"User-Agent", "KeePassXC"});
    }
    return NetworkRequest(std::move(target), maxRedirects, timeoutDuration, additionalHeaders, manager);
}
