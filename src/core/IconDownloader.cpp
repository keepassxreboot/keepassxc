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
#include "core/NetworkManager.h"

#include <QHostInfo>
#include <QImageReader>
#include <QtNetwork>

#define MAX_REDIRECTS 5

IconDownloader::IconDownloader(QObject* parent)
    : QObject(parent)
    , m_reply(nullptr)
    , m_redirects(0)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, SIGNAL(timeout()), SLOT(abortDownload()));
}

IconDownloader::~IconDownloader()
{
    abortDownload();
}

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
        if (var.canConvert<QUrl>()) {
            url = var.toUrl();
        }
        return url;
    }

    QUrl getRedirectTarget(QNetworkReply* reply)
    {
        QVariant var = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        QUrl url = convertVariantToUrl(var);
        return url;
    }
} // namespace

void IconDownloader::setUrl(const QString& entryUrl)
{
    m_url = entryUrl;
    QUrl url(m_url);
    if (!url.isValid()) {
        return;
    }

    m_redirects = 0;
    m_urlsToTry.clear();

    if (url.scheme().isEmpty()) {
        url.setUrl(QString("https://%1").arg(url.toString()));
    }

    QString fullyQualifiedDomain = url.host();

    // Determine if host portion of URL is an IP address by resolving it and
    // searching for a match with the returned address(es).
    bool hostIsIp = false;
    QList<QHostAddress> hostAddressess = QHostInfo::fromName(fullyQualifiedDomain).addresses();
    for (const auto& addr : hostAddressess) {
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
    if (config()->get(Config::Security_IconDownloadFallback).toBool()) {
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
    m_urlsToTry.append(QUrl(url.scheme() + "://" + fullyQualifiedDomain + "/favicon.ico"));

    // Also try a direct pull of the second-level domain (if possible)
    if (!hostIsIp && fullyQualifiedDomain != secondLevelDomain) {
        m_urlsToTry.append(QUrl(url.scheme() + "://" + secondLevelDomain + "/favicon.ico"));
    }
}

void IconDownloader::download()
{
    if (m_urlsToTry.isEmpty()) {
        return;
    }

    if (!m_timeout.isActive()) {
        int timeout = config()->get(Config::FaviconDownloadTimeout).toInt();
        m_timeout.start(timeout * 1000);

        // Use the first URL to start the download process
        // If a favicon is not found, the next URL will be tried
        fetchFavicon(m_urlsToTry.takeFirst());
    }
}

void IconDownloader::abortDownload()
{
    if (m_reply) {
        m_reply->abort();
    }
}

void IconDownloader::fetchFavicon(const QUrl& url)
{
    m_bytesReceived.clear();
    m_fetchUrl = url;

    QNetworkRequest request(url);
    m_reply = getNetMgr()->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &IconDownloader::fetchFinished);
    connect(m_reply, &QIODevice::readyRead, this, &IconDownloader::fetchReadyRead);
}

void IconDownloader::fetchReadyRead()
{
    m_bytesReceived += m_reply->readAll();
}

void IconDownloader::fetchFinished()
{
    QImage image;
    QString url = m_url;

    bool error = (m_reply->error() != QNetworkReply::NoError);
    QUrl redirectTarget = getRedirectTarget(m_reply);

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!error) {
        if (redirectTarget.isValid()) {
            // Redirected, we need to follow it, or fall through if we have
            // done too many redirects already.
            if (m_redirects < MAX_REDIRECTS) {
                m_redirects++;
                if (redirectTarget.isRelative()) {
                    redirectTarget = m_fetchUrl.resolved(redirectTarget);
                }
                m_urlsToTry.prepend(redirectTarget);
            }
        } else {
            // No redirect, and we theoretically have some icon data now.
            image = parseImage(m_bytesReceived);
        }
    }

    if (!image.isNull()) {
        // Valid icon received
        m_timeout.stop();
        emit finished(url, image);
    } else if (!m_urlsToTry.empty()) {
        // Try the next url
        m_redirects = 0;
        fetchFavicon(m_urlsToTry.takeFirst());
    } else {
        // No icon found
        m_timeout.stop();
        emit finished(url, image);
    }
}

/**
 * Parse fetched image bytes.
 *
 * Parses the given byte array into a QImage. Unlike QImage::loadFromData(), this method
 * tries to extract the highest resolution image from .ICO files.
 *
 * @param imageBytes raw image bytes
 * @return parsed image
 */
QImage IconDownloader::parseImage(QByteArray& imageBytes) const
{
    QBuffer buff(&imageBytes);
    buff.open(QIODevice::ReadOnly);
    QImageReader reader(&buff);

    if (reader.imageCount() <= 0) {
        return reader.read();
    }

    QImage img;
    for (int i = 0; i < reader.imageCount(); ++i) {
        if (img.isNull() || reader.size().width() > img.size().width()) {
            img = reader.read();
        }
        reader.jumpToNextImage();
    }

    return img;
}
