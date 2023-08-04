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

#ifndef DATABASESETTINGS_H
#define DATABASESETTINGS_H

#include "core/Database.h"
#include <QObject>

class DatabaseSettings : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(DatabaseSettings)

    explicit DatabaseSettings() = default;
    ;
    static DatabaseSettings* instance();

    bool getAlwaysAllowAccess(const QSharedPointer<Database>& db);
    void setAlwaysAllowAccess(const QSharedPointer<Database>& db, bool enabled);
    bool getAllowGetDatabaseEntriesRequest(const QSharedPointer<Database>& db);
    void setAllowGetDatabaseEntriesRequest(const QSharedPointer<Database>& db, bool enabled);

    static const QString OPTION_ALLOW_GET_DATABASE_ENTRIES_REQUEST;
    static const QString OPTION_ALWAYS_ALLOW_ACCESS;

private:
    QString getCustomDataOption(const QSharedPointer<Database>& db, const QString& key) const;
    void setCustomDataOption(const QSharedPointer<Database>& db, const QString& key, const QString& value) const;
};

static inline DatabaseSettings* databaseSettings()
{
    return DatabaseSettings::instance();
}

#endif // DATABASESETTINGS_H
