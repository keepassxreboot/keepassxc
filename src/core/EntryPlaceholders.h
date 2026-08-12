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

#ifndef KEEPASSXC_ENTRYPLACEHOLDERS_H
#define KEEPASSXC_ENTRYPLACEHOLDERS_H

#include <QRegularExpression>
#include <QString>

namespace EntryPlaceholders
{
    enum class PlaceholderType
    {
        NotPlaceholder,
        Unknown,
        Title,
        UserName,
        Password,
        Notes,
        Totp,
        Url,
        Uuid,
        UrlWithoutScheme,
        UrlScheme,
        UrlHost,
        UrlPort,
        UrlPath,
        UrlQuery,
        UrlFragment,
        UrlUserInfo,
        UrlUserName,
        UrlPassword,
        Reference,
        CustomAttribute,
        DateTimeSimple,
        DateTimeYear,
        DateTimeMonth,
        DateTimeDay,
        DateTimeHour,
        DateTimeMinute,
        DateTimeSecond,
        DateTimeUtcSimple,
        DateTimeUtcYear,
        DateTimeUtcMonth,
        DateTimeUtcDay,
        DateTimeUtcHour,
        DateTimeUtcMinute,
        DateTimeUtcSecond,
        DbDir,
        Conversion,
        Regex
    };

    PlaceholderType placeholderType(const QString& placeholder);
    QString resolveUrlPlaceholder(const QString& str, PlaceholderType placeholderType);
    QString resolveDateTimePlaceholder(PlaceholderType placeholderType);
    QString maskPasswordPlaceholders(const QString& str);
    QRegularExpressionMatchIterator placeholderMatches(const QString& str);
    bool containsPlaceholder(const QString& str);
} // namespace EntryPlaceholders

#endif // KEEPASSXC_ENTRYPLACEHOLDERS_H
