/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_URLTOOLS_H
#define KEEPASSXC_URLTOOLS_H

#include "config-keepassx.h"
#include <QString>
#if defined(KPXC_FEATURE_NETWORK) || defined(KPXC_FEATURE_BROWSER)
#include <QUrl>
class QNetworkReply;
#endif

namespace UrlTools
{
#if defined(KPXC_FEATURE_NETWORK) || defined(KPXC_FEATURE_BROWSER)
    QUrl getRedirectTarget(QNetworkReply* reply);
    QString getBaseDomainFromUrl(const QString& url);
    QString getTopLevelDomainFromUrl(const QString& url);
    bool isIpAddress(const QString& host);
#endif
    bool isUrlIdentical(QString first, QString second);
    bool isUrlValid(const QString& urlField, bool looseComparison = false);
    bool domainHasIllegalCharacters(const QString& domain);

    extern const QString URL_WILDCARD;
} // namespace UrlTools

#endif // KEEPASSXC_URLTOOLS_H
