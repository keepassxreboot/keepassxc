
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
} // namespace

void NetworkRequest::fetch(const QUrl& url)
{
    reset();

    m_url = url;

    QNetworkRequest request(url);

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
    auto error = m_reply->error();
    QUrl redirectTarget = getRedirectTarget(m_reply);
    QUrl url = m_reply->url();
    // Returns an empty string if the header was not set
    QString contentTypeHeader = m_reply->header(QNetworkRequest::ContentTypeHeader).toString();

    m_reply->deleteLater();
    m_reply = nullptr;

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
            fetch(redirectTarget);
            return;
        } else {
            emit failure();
            return;
        }
    }

    // Parse content type
    auto tokens = contentTypeHeader.split(";", Qt::SkipEmptyParts);
    m_content_type = tokens[0].trimmed();
    for (int i = 1; i < tokens.size(); ++i) {
        auto parameterTokens = tokens[i].split("=");
        m_content_type_parameters[parameterTokens[0]] = parameterTokens[1];
    }

    emit success(std::move(m_bytes));
}

void NetworkRequest::fetchReadyRead()
{
    m_bytes += m_reply->readAll();
}

void NetworkRequest::reset()
{
    m_redirects = 0;
    m_bytes.clear();
    m_content_type = "";
    m_content_type_parameters.clear();
    m_timeout.setInterval(m_timeoutDuration);
    m_timeout.setSingleShot(true);
}

void NetworkRequest::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
}

NetworkRequest::~NetworkRequest()
{
    cancel();
}

QUrl NetworkRequest::url() const
{
    return m_url;
}

void NetworkRequest::setMaxRedirects(int maxRedirects)
{
    m_maxRedirects = std::max(0, maxRedirects);
}

NetworkRequest::NetworkRequest(int maxRedirects,
                               std::chrono::milliseconds timeoutDuration,
                               QList<QPair<QString, QString>> headers,
                               QNetworkAccessManager* manager)
    : m_reply(nullptr)
    , m_maxRedirects(maxRedirects)
    , m_redirects(0)
    , m_timeoutDuration(timeoutDuration)
    , m_headers(headers)
{
    m_manager = manager ? manager : getNetMgr();
    connect(&m_timeout, &QTimer::timeout, this, &NetworkRequest::fetchTimeout);
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
    // Cancel request on timeout
    cancel();
}
NetworkRequest createRequest(int maxRedirects,
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
    return NetworkRequest(maxRedirects, timeoutDuration, additionalHeaders, manager);
}
