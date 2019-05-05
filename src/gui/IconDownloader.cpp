/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "IconDownloader.h"
#include "core/Config.h"

#ifdef WITH_XC_NETWORKING
#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QtNetwork>
#endif

IconDownloader::IconDownloader(QObject* parent)
    : QObject(parent)
#ifdef WITH_XC_NETWORKING
    , m_netMgr(new QNetworkAccessManager(this))
    , m_reply(nullptr)
#endif
{
}

IconDownloader::~IconDownloader()
{
}

#ifdef WITH_XC_NETWORKING
namespace
{
    // Try to get the 2nd level domain of the host part of a QUrl. For example,
    // "foo.bar.example.com" would become "example.com", and "foo.bar.example.co.uk"
    // would become "example.co.uk".
    QString getSecondLevelDomain(const QUrl& url)
    {
        QString fqdn = url.host();
        fqdn.truncate(fqdn.length() - url.topLevelDomain().length());
        QStringList parts = fqdn.split('.');
        QString newdom = parts.takeLast() + url.topLevelDomain();
        return newdom;
    }

    QUrl convertVariantToUrl(const QVariant& var)
    {
        QUrl url;
        if (var.canConvert<QUrl>())
            url = var.toUrl();
        return url;
    }

    QUrl getRedirectTarget(QNetworkReply* reply)
    {
        QVariant var = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        QUrl url = convertVariantToUrl(var);
        return url;
    }
} // namespace
#endif

void IconDownloader::downloadFavicon(Entry* entry)
{
#ifdef WITH_XC_NETWORKING
    m_entry = entry;
    m_url = entry->url();
    m_redirects = 0;
    m_urlsToTry.clear();

    if (m_url.scheme().isEmpty()) {
        m_url.setUrl(QString("https://%1").arg(m_url.toString()));
    }

    QString fullyQualifiedDomain = m_url.host();

    // Determine if host portion of URL is an IP address by resolving it and
    // searching for a match with the returned address(es).
    bool hostIsIp = false;
    QList<QHostAddress> hostAddressess = QHostInfo::fromName(fullyQualifiedDomain).addresses();
    for (auto addr : hostAddressess) {
        if (addr.toString() == fullyQualifiedDomain) {
            hostIsIp = true;
        }
    }

    // Determine the second-level domain, if available
    QString secondLevelDomain;
    if (!hostIsIp) {
        secondLevelDomain = getSecondLevelDomain(m_url);
    }

    // Start with the "fallback" url (if enabled) to try to get the best favicon
    if (config()->get("security/IconDownloadFallback", false).toBool()) {
        QUrl fallbackUrl = QUrl("https://icons.duckduckgo.com");
        fallbackUrl.setPath("/ip3/" + QUrl::toPercentEncoding(fullyQualifiedDomain) + ".ico");
        m_urlsToTry.append(fallbackUrl);

        // Also try a direct pull of the second-level domain (if possible)
        if (!hostIsIp && fullyQualifiedDomain != secondLevelDomain) {
            fallbackUrl.setPath("/ip3/" + QUrl::toPercentEncoding(secondLevelDomain) + ".ico");
            m_urlsToTry.append(fallbackUrl);
        }
    }

    // Add a direct pull of the website's own favicon.ico file
    m_urlsToTry.append(QUrl(m_url.scheme() + "://" + fullyQualifiedDomain + "/favicon.ico"));

    // Also try a direct pull of the second-level domain (if possible)
    if (!hostIsIp && fullyQualifiedDomain != secondLevelDomain) {
        m_urlsToTry.append(QUrl(m_url.scheme() + "://" + secondLevelDomain + "/favicon.ico"));
    }

    // Use the first URL to start the download process
    // If a favicon is not found, the next URL will be tried
    startFetchFavicon(m_urlsToTry.takeFirst());
#else
    Q_UNUSED(entry);
#endif
}

void IconDownloader::fetchReadyRead()
{
#ifdef WITH_XC_NETWORKING
    m_bytesReceived += m_reply->readAll();
#endif
}

void IconDownloader::fetchFinished()
{
#ifdef WITH_XC_NETWORKING
    QImage image;
    bool fallbackEnabled = config()->get("security/IconDownloadFallback", false).toBool();
    bool error = (m_reply->error() != QNetworkReply::NoError);
    if (m_reply->error() == QNetworkReply::HostNotFoundError || m_reply->error() == QNetworkReply::TimeoutError) {
        emit iconReceived(image, m_entry);
        return;
    }
    QUrl redirectTarget = getRedirectTarget(m_reply);

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!error) {
        if (redirectTarget.isValid()) {
            // Redirected, we need to follow it, or fall through if we have
            // done too many redirects already.
            if (m_redirects < 5) {
                m_redirects++;
                if (redirectTarget.isRelative())
                    redirectTarget = m_fetchUrl.resolved(redirectTarget);
                startFetchFavicon(redirectTarget);
                return;
            }
        } else {
            // No redirect, and we theoretically have some icon data now.
            image.loadFromData(m_bytesReceived);
        }
    }

    if (!image.isNull()) {
        emit iconReceived(image, m_entry);
        return;
    } else if (!m_urlsToTry.empty()) {
        m_redirects = 0;
        startFetchFavicon(m_urlsToTry.takeFirst());
        return;
    } else {
        if (!fallbackEnabled) {
            emit fallbackNotEnabled();
        } else {
            emit iconError(m_entry);
        }
    }
    emit iconReceived(image, m_entry);
#endif
}

void IconDownloader::abortRequest()
{
#ifdef WITH_XC_NETWORKING
    if (m_reply) {
        m_reply->abort();
    }
#endif
}

void IconDownloader::startFetchFavicon(const QUrl& url)
{
#ifdef WITH_XC_NETWORKING
    m_bytesReceived.clear();
    m_fetchUrl = url;

    QNetworkRequest request(url);
    m_reply = m_netMgr->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &IconDownloader::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &IconDownloader::fetchReadyRead);
#else
    Q_UNUSED(url);
#endif
}
