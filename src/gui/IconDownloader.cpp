/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#include "core/UrlTools.h"

#include <QBuffer>
#include <QHostInfo>
#include <QImageReader>
#include <QNetworkReply>

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

void IconDownloader::setUrl(const QString& entryUrl)
{
    m_url = entryUrl;
    QUrl url = QUrl::fromUserInput(m_url);
    if (!url.isValid() || url.host().isEmpty()) {
        return;
    }

    m_redirects = 0;
    m_urlsToTry.clear();

    // Fall back to https if no scheme is specified
    // fromUserInput defaults to http. Hence, we need to replace the default scheme should we detect that it has
    // been added by fromUserInput
    if (!entryUrl.startsWith(url.scheme())) {
        url.setScheme("https");
    } else if (url.scheme() != "https" && url.scheme() != "http") {
        return;
    }

    // Remove query string - if any
    if (url.hasQuery()) {
        url.setQuery(QString());
    }
    // Remove fragment - if any
    if (url.hasFragment()) {
        url.setFragment(QString());
    }

    QString fullyQualifiedDomain = url.host();

    // Determine if host portion of URL is an IP address by resolving it and
    // searching for a match with the returned address(es).
    bool hostIsIp = false;
    QList<QHostAddress> hostAddressess = QHostInfo::fromName(fullyQualifiedDomain).addresses();
    hostIsIp =
        std::any_of(hostAddressess.begin(), hostAddressess.end(), [&fullyQualifiedDomain](const QHostAddress& addr) {
            return addr.toString() == fullyQualifiedDomain;
        });

    // Determine the second-level domain, if available
    QString secondLevelDomain;
    if (!hostIsIp) {
        secondLevelDomain = urlTools()->getBaseDomainFromUrl(url.toString());
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

    // Add a pull that preserves the query if there is one.
    if (!url.path().isEmpty()) {
        // Appends /favicon.ico to the last segment of the path.
        // stem/something/ will become stem/something/favicon.ico, and stem/something will become stem/favicon.ico
        m_urlsToTry.append(url.resolved(QUrl("./favicon.ico")));
    }

    // Add a direct pull of the website's own favicon.ico file
    QUrl favicon_url = url;
    favicon_url.setPath("/favicon.ico");
    m_urlsToTry.append(favicon_url);

    // Also try a direct pull of the second-level domain (if possible)
    if (!hostIsIp && fullyQualifiedDomain != secondLevelDomain && !secondLevelDomain.isEmpty()) {
        favicon_url.setHost(secondLevelDomain);
        m_urlsToTry.append(favicon_url);
        if (!favicon_url.userInfo().isEmpty() || favicon_url.port() != -1) {
            // Remove additional fields from the URL as a fallback. Keep only host and scheme
            // Fragment and query have been removed earlier
            favicon_url.setPort(-1);
            favicon_url.setUserInfo(QString());
            m_urlsToTry.append(favicon_url);
        }
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
    QUrl redirectTarget = urlTools()->getRedirectTarget(m_reply);

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
