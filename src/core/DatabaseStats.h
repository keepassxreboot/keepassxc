/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASESTATS_H
#define KEEPASSXC_DATABASESTATS_H
#include "PasswordHealth.h"
#include "core/Group.h"
#include <QFileInfo>
#include <cmath>
class DatabaseStats
{
public:
    // The statistics we collect:
    QDateTime modified; // File modification time
    int groupCount = 0; // Number of groups in the database
    int entryCount = 0; // Number of entries (across all groups)
    int expiredEntries = 0; // Number of expired entries
    int excludedEntries = 0; // Number of known bad entries
    int weakPasswords = 0; // Number of weak or poor passwords
    int shortPasswords = 0; // Number of passwords 8 characters or less in size
    int uniquePasswords = 0; // Number of unique passwords
    int reusedPasswords = 0; // Number of non-unique passwords
    int totalPasswordLength = 0; // Total length of all passwords

    explicit DatabaseStats(QSharedPointer<Database> db);

    int averagePwdLength() const;

    int maxPwdReuse() const;

    bool isAnyExpired() const;

    bool areTooManyPwdsReused() const;

    bool arePwdsReusedTooOften() const;

    bool isAvgPwdTooShort() const;

private:
    QSharedPointer<Database> m_db;
    QHash<QString, int> m_passwords;

    void gatherStats(const QList<Group*>& groups);
};
#endif // KEEPASSXC_DATABASESTATS_H
