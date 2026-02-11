/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AUTOFILL_UTILS_H
#define KEEPASSX_AUTOFILL_UTILS_H

#include "gui/UrlTools.h"

#include <QRegularExpression>
#include <QString>
#include <QUrl>

namespace AutoFillUtils
{

inline QString normalizeHost(const QString& host)
{
    auto normalized = host.trimmed().toLower();
    while (normalized.endsWith('.')) {
        normalized.chop(1);
    }
    return normalized;
}

inline QString hostFromUrl(const QString& value)
{
    if (value.trimmed().isEmpty()) {
        return {};
    }

    QUrl url = QUrl::fromUserInput(value.trimmed());
    QString host = url.host().trimmed();
    if (host.isEmpty() && !value.contains('/')) {
        host = value;
    }

    host = normalizeHost(host);
    if (host.isEmpty()) {
        return {};
    }

    static const QRegularExpression kDomainRegex(QStringLiteral("^[a-z0-9.-]+$"));
    if (!kDomainRegex.match(host).hasMatch()) {
        return {};
    }

    return host;
}

inline bool hostsMatch(const QString& requested, const QString& candidate)
{
    if (requested.isEmpty() || candidate.isEmpty()) {
        return false;
    }

    // Reject hosts that start with a dot (potential security issue)
    if (requested.startsWith('.') || candidate.startsWith('.')) {
        return false;
    }

    if (requested == candidate) {
        return true;
    }

    // IP addresses require exact match only (already handled above)
    if (urlTools()->isIpAddress(requested) || urlTools()->isIpAddress(candidate)) {
        return false;
    }

    // Base domains must match (follows BrowserService::handleURL pattern)
    if (urlTools()->getBaseDomainFromUrl(requested) != urlTools()->getBaseDomainFromUrl(candidate)) {
        return false;
    }

    // Allow subdomain matching when one host ends with the other
    if (requested.endsWith('.' + candidate) || candidate.endsWith('.' + requested)) {
        return true;
    }

    return false;
}

} // namespace AutoFillUtils

#endif // KEEPASSX_AUTOFILL_UTILS_H
