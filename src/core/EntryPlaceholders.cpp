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

#include "EntryPlaceholders.h"

#include <QDateTime>
#include <QMap>
#include <QStringLiteral>
#include <QUrl>

#include "Clock.h"

namespace EntryPlaceholders
{
    PlaceholderType placeholderType(const QString& placeholder)
    {
        if (!placeholder.startsWith(QStringLiteral("{")) || !placeholder.endsWith(QStringLiteral("}"))) {
            return PlaceholderType::NotPlaceholder;
        }
        if (placeholder.startsWith(QStringLiteral("{S:"))) {
            return PlaceholderType::CustomAttribute;
        }
        if (placeholder.startsWith(QStringLiteral("{REF:"))) {
            return PlaceholderType::Reference;
        }
        if (placeholder.startsWith(QStringLiteral("{T-CONV:"), Qt::CaseInsensitive)) {
            return PlaceholderType::Conversion;
        }
        if (placeholder.startsWith(QStringLiteral("{T-REPLACE-RX:"), Qt::CaseInsensitive)) {
            return PlaceholderType::Regex;
        }

        static const QMap<QString, PlaceholderType> placeholders{
            {QStringLiteral("{TITLE}"), PlaceholderType::Title},
            {QStringLiteral("{USERNAME}"), PlaceholderType::UserName},
            {QStringLiteral("{PASSWORD}"), PlaceholderType::Password},
            {QStringLiteral("{NOTES}"), PlaceholderType::Notes},
            {QStringLiteral("{TOTP}"), PlaceholderType::Totp},
            {QStringLiteral("{TIMEOTP}"), PlaceholderType::Totp},
            {QStringLiteral("{URL}"), PlaceholderType::Url},
            {QStringLiteral("{UUID}"), PlaceholderType::Uuid},
            {QStringLiteral("{URL:RMVSCM}"), PlaceholderType::UrlWithoutScheme},
            {QStringLiteral("{URL:WITHOUTSCHEME}"), PlaceholderType::UrlWithoutScheme},
            {QStringLiteral("{URL:SCM}"), PlaceholderType::UrlScheme},
            {QStringLiteral("{URL:SCHEME}"), PlaceholderType::UrlScheme},
            {QStringLiteral("{URL:HOST}"), PlaceholderType::UrlHost},
            {QStringLiteral("{URL:PORT}"), PlaceholderType::UrlPort},
            {QStringLiteral("{URL:PATH}"), PlaceholderType::UrlPath},
            {QStringLiteral("{URL:QUERY}"), PlaceholderType::UrlQuery},
            {QStringLiteral("{URL:FRAGMENT}"), PlaceholderType::UrlFragment},
            {QStringLiteral("{URL:USERINFO}"), PlaceholderType::UrlUserInfo},
            {QStringLiteral("{URL:USERNAME}"), PlaceholderType::UrlUserName},
            {QStringLiteral("{URL:PASSWORD}"), PlaceholderType::UrlPassword},
            {QStringLiteral("{DT_SIMPLE}"), PlaceholderType::DateTimeSimple},
            {QStringLiteral("{DT_YEAR}"), PlaceholderType::DateTimeYear},
            {QStringLiteral("{DT_MONTH}"), PlaceholderType::DateTimeMonth},
            {QStringLiteral("{DT_DAY}"), PlaceholderType::DateTimeDay},
            {QStringLiteral("{DT_HOUR}"), PlaceholderType::DateTimeHour},
            {QStringLiteral("{DT_MINUTE}"), PlaceholderType::DateTimeMinute},
            {QStringLiteral("{DT_SECOND}"), PlaceholderType::DateTimeSecond},
            {QStringLiteral("{DT_UTC_SIMPLE}"), PlaceholderType::DateTimeUtcSimple},
            {QStringLiteral("{DT_UTC_YEAR}"), PlaceholderType::DateTimeUtcYear},
            {QStringLiteral("{DT_UTC_MONTH}"), PlaceholderType::DateTimeUtcMonth},
            {QStringLiteral("{DT_UTC_DAY}"), PlaceholderType::DateTimeUtcDay},
            {QStringLiteral("{DT_UTC_HOUR}"), PlaceholderType::DateTimeUtcHour},
            {QStringLiteral("{DT_UTC_MINUTE}"), PlaceholderType::DateTimeUtcMinute},
            {QStringLiteral("{DT_UTC_SECOND}"), PlaceholderType::DateTimeUtcSecond},
            {QStringLiteral("{DB_DIR}"), PlaceholderType::DbDir}};

        return placeholders.value(placeholder.toUpper(), PlaceholderType::Unknown);
    }

    QString resolveUrlPlaceholder(const QString& str, PlaceholderType placeholderType)
    {
        if (str.isEmpty()) {
            return {};
        }

        const QUrl qurl(str);
        switch (placeholderType) {
        case PlaceholderType::UrlWithoutScheme:
            return qurl.toString(QUrl::RemoveScheme | QUrl::FullyDecoded);
        case PlaceholderType::UrlScheme:
            return qurl.scheme();
        case PlaceholderType::UrlHost:
            return qurl.host();
        case PlaceholderType::UrlPort:
            return QString::number(qurl.port());
        case PlaceholderType::UrlPath:
            return qurl.path();
        case PlaceholderType::UrlQuery:
            return qurl.query();
        case PlaceholderType::UrlFragment:
            return qurl.fragment();
        case PlaceholderType::UrlUserInfo:
            return qurl.userInfo();
        case PlaceholderType::UrlUserName:
            return qurl.userName();
        case PlaceholderType::UrlPassword:
            return qurl.password();
        default: {
            Q_ASSERT_X(false, "EntryPlaceholders::resolveUrlPlaceholder", "Bad url placeholder type");
            break;
        }
        }

        return {};
    }

    QString resolveDateTimePlaceholder(PlaceholderType placeholderType)
    {
        const QDateTime time = Clock::currentDateTime();
        const QDateTime time_utc = Clock::currentDateTimeUtc();

        switch (placeholderType) {
        case PlaceholderType::DateTimeSimple:
            return time.toString("yyyyMMddhhmmss");
        case PlaceholderType::DateTimeYear:
            return time.toString("yyyy");
        case PlaceholderType::DateTimeMonth:
            return time.toString("MM");
        case PlaceholderType::DateTimeDay:
            return time.toString("dd");
        case PlaceholderType::DateTimeHour:
            return time.toString("hh");
        case PlaceholderType::DateTimeMinute:
            return time.toString("mm");
        case PlaceholderType::DateTimeSecond:
            return time.toString("ss");
        case PlaceholderType::DateTimeUtcSimple:
            return time_utc.toString("yyyyMMddhhmmss");
        case PlaceholderType::DateTimeUtcYear:
            return time_utc.toString("yyyy");
        case PlaceholderType::DateTimeUtcMonth:
            return time_utc.toString("MM");
        case PlaceholderType::DateTimeUtcDay:
            return time_utc.toString("dd");
        case PlaceholderType::DateTimeUtcHour:
            return time_utc.toString("hh");
        case PlaceholderType::DateTimeUtcMinute:
            return time_utc.toString("mm");
        case PlaceholderType::DateTimeUtcSecond:
            return time_utc.toString("ss");
        default: {
            Q_ASSERT_X(false, "EntryPlaceholders::resolveDateTimePlaceholder", "Bad DateTime placeholder type");
            break;
        }
        }

        return {};
    }

    QString maskPasswordPlaceholders(const QString& str)
    {
        return QString{str}.replace(QStringLiteral("{PASSWORD}"), QStringLiteral("******"), Qt::CaseInsensitive);
    }

    QRegularExpressionMatchIterator placeholderMatches(const QString& str)
    {
        static const QRegularExpression placeholderRegEx("({(?>[^{}]+?|(?1))+?})");
        return placeholderRegEx.globalMatch(str);
    }

    bool containsPlaceholder(const QString& str)
    {
        auto matches = placeholderMatches(str);
        while (matches.hasNext()) {
            const auto match = matches.next();
            auto captured = match.captured(0);

            // Remove escaped brackets
            if (captured.startsWith("\\{")) {
                captured.replace(0, 2, "{");
            }
            if (captured.endsWith("\\}")) {
                captured.replace(captured.size() - 2, 2, "}");
            }

            const auto placeHolderType = placeholderType(captured);
            if (placeHolderType != PlaceholderType::NotPlaceholder && placeHolderType != PlaceholderType::Unknown) {
                return true;
            }
        }
        return false;
    }
} // namespace EntryPlaceholders
