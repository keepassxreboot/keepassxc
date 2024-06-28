/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DatabaseSettings.h"
#include "core/CustomData.h"
#include "core/Global.h"
#include "core/Metadata.h"

Q_GLOBAL_STATIC(DatabaseSettings, s_databaseSettings);

DatabaseSettings* DatabaseSettings::instance()
{
    return s_databaseSettings;
}

const QString DatabaseSettings::OPTION_ALLOW_GET_DATABASE_ENTRIES_REQUEST =
    QStringLiteral("BrowserAllowGetDatabaseEntriesRequest");
const QString DatabaseSettings::OPTION_ALWAYS_ALLOW_ACCESS = QStringLiteral("BrowserAlwaysAllowAccess");

bool DatabaseSettings::getAlwaysAllowAccess(const QSharedPointer<Database>& db)
{
    return getCustomDataOption(db, DatabaseSettings::OPTION_ALWAYS_ALLOW_ACCESS) == TRUE_STR;
}

void DatabaseSettings::setAlwaysAllowAccess(const QSharedPointer<Database>& db, bool enabled)
{
    setCustomDataOption(db, DatabaseSettings::OPTION_ALWAYS_ALLOW_ACCESS, enabled ? TRUE_STR : FALSE_STR);
}

bool DatabaseSettings::getAllowGetDatabaseEntriesRequest(const QSharedPointer<Database>& db)
{
    return getCustomDataOption(db, DatabaseSettings::OPTION_ALLOW_GET_DATABASE_ENTRIES_REQUEST) == TRUE_STR;
}

void DatabaseSettings::setAllowGetDatabaseEntriesRequest(const QSharedPointer<Database>& db, bool enabled)
{
    setCustomDataOption(
        db, DatabaseSettings::OPTION_ALLOW_GET_DATABASE_ENTRIES_REQUEST, enabled ? TRUE_STR : FALSE_STR);
}

QString DatabaseSettings::getCustomDataOption(const QSharedPointer<Database>& db, const QString& key) const
{
    if (!db) {
        return {};
    }

    return db->metadata()->customData()->value(CustomData::OptionPrefix + key);
}

void DatabaseSettings::setCustomDataOption(const QSharedPointer<Database>& db,
                                           const QString& key,
                                           const QString& value) const
{
    if (!db) {
        return;
    }

    db->metadata()->customData()->set(CustomData::OptionPrefix + key, value);
}
