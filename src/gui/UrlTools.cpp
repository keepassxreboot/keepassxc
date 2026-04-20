/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "UrlTools.h"
#if defined(KPXC_FEATURE_NETWORK) || defined(KPXC_FEATURE_BROWSER)
#include <QHostAddress>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QVariant>
#endif
#include <QRegularExpression>
#include <QUrl>

#include <utility>

const QString UrlTools::URL_WILDCARD = "1kpxcwc1";

#if defined(KPXC_FEATURE_NETWORK) || defined(KPXC_FEATURE_BROWSER)
QUrl UrlTools::getRedirectTarget(QNetworkReply* reply)
{
    QUrl url;
    QVariant var = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (var.canConvert<QUrl>()) {
        url = var.toUrl();
    }
    return url;
}

/**
 * Gets the base domain of URL or hostname.
 *
 * Returns the base domain, e.g. https://another.example.co.uk -> example.co.uk
 * Up-to-date list can be found: https://publicsuffix.org/list/public_suffix_list.dat
 */
QString UrlTools::getBaseDomainFromUrl(const QString& url)
{
    auto qUrl = QUrl::fromUserInput(url);

    auto host = qUrl.host();
    if (isIpAddress(host)) {
        return host;
    }

    const auto tld = getTopLevelDomainFromUrl(qUrl.toString());
    if (tld.isEmpty() || tld.length() + 1 >= host.length()) {
        return host;
    }

    // Remove the top level domain part from the hostname, e.g. https://another.example.co.uk -> https://another.example
    host.chop(tld.length() + 1);
    // Split the URL and select the last part, e.g. https://another.example -> example
    QString baseDomain = host.split('.').last();
    // Append the top level domain back to the URL, e.g. example -> example.co.uk
    baseDomain.append(QString(".%1").arg(tld));

    return baseDomain;
}

/**
 * Gets the top level domain from URL.
 *
 * Returns the TLD e.g. https://another.example.co.uk -> co.uk
 */
QString UrlTools::getTopLevelDomainFromUrl(const QString& url)
{
    auto host = QUrl::fromUserInput(url).host();
    if (isIpAddress(host)) {
        return host;
    }

    const auto numberOfDomainParts = host.split('.').length();
    static const auto dummy = QByteArrayLiteral("");

    // Only loop the amount of different parts found
    for (auto i = 0; i < numberOfDomainParts; ++i) {
        // Cut the first part from host
        host = host.mid(host.indexOf('.') + 1);

        QNetworkCookie cookie(dummy, dummy);
        cookie.setDomain(host);

        // Check if dummy cookie's domain/TLD matches with public suffix list
        if (!QNetworkCookieJar{}.setCookiesFromUrl(QList{cookie}, QUrl::fromUserInput(url))) {
            return host;
        }
    }

    return host;
}

bool UrlTools::isIpAddress(const QString& host)
{
    // Handle IPv6 host with brackets, e.g [::1]
    const auto hostAddress = host.startsWith('[') && host.endsWith(']') ? host.mid(1, host.length() - 2) : host;
    QHostAddress address(hostAddress);
    return address.protocol() == QAbstractSocket::IPv4Protocol || address.protocol() == QAbstractSocket::IPv6Protocol;
}
#endif

namespace
{
    QString trimUrl(QString& url)
    {
        url = url.trimmed().replace("*", UrlTools::URL_WILDCARD);
        if (url.endsWith('/')) {
            url.chop(1); // Removes the last character
        }
        return url;
    }
} // namespace

// Returns true if URLs are identical. Paths with "/" are removed during comparison.
// URLs without scheme reverts to https.
// Special handling is needed because QUrl::matches() with QUrl::StripTrailingSlash does not strip "/" paths.
bool UrlTools::isUrlIdentical(QString first, QString second)
{
    if (first.isEmpty() || second.isEmpty()) {
        return false;
    }

    // Replace URL wildcards for comparison if found
    const auto firstUrl = trimUrl(first);
    const auto secondUrl = trimUrl(second);
    if (firstUrl == secondUrl) {
        return true;
    }

    return QUrl(firstUrl).matches(QUrl(secondUrl), QUrl::StripTrailingSlash);
}

bool UrlTools::isUrlValid(const QString& urlField, bool looseComparison)
{
    if (urlField.isEmpty() || urlField.startsWith("cmd://", Qt::CaseInsensitive)
        || urlField.startsWith("kdbx://", Qt::CaseInsensitive) || urlField.startsWith("{REF:A", Qt::CaseInsensitive)) {
        return true;
    }

    auto url = urlField;

    // Loose comparison that allows wildcards and exact URL inside " characters
    if (looseComparison) {
        // Exact URL
        if (url.startsWith("\"") && url.endsWith("\"")) {
            // Do not allow exact URL with wildcards, or empty exact URL
            if (url.contains("*") || url.length() == 2) {
                return false;
            }

            // Get the URL inside ""
            url.remove(0, 1);
            url.chop(1);
        } else {
            // Do not allow URL with just wildcards, or double wildcards
            if (url.length() == url.count("*") || url.contains("**") || url.contains("*.*")) {
                return false;
            }

            url.replace("*", URL_WILDCARD);
        }
    }

    QUrl qUrl;
    if (urlField.contains("://")) {
        qUrl = url;
    } else {
        qUrl = QUrl::fromUserInput(url);
    }

    if (qUrl.scheme() != "file" && qUrl.host().isEmpty()) {
        return false;
    }

#if defined(KPXC_FEATURE_BROWSER)
    // Prevent TLD wildcards
    if (looseComparison && url.contains(URL_WILDCARD)) {
        const auto tld = getTopLevelDomainFromUrl(url);
        if (tld.contains(URL_WILDCARD) || qUrl.host() == QString("%1.%2").arg(URL_WILDCARD, tld)) {
            return false;
        }
    }
#endif

    // Check for illegal characters. Adds also the wildcard * to the list
    static const QRegularExpression re("[<>\\^`{|}\\*]");
    auto match = re.match(url);
    if (match.hasMatch()) {
        return false;
    }

    return true;
}

bool UrlTools::domainHasIllegalCharacters(const QString& domain)
{
    static const QRegularExpression re(R"([\s\^#|/:<>\?@\[\]\\])");
    return re.match(domain).hasMatch();
}
