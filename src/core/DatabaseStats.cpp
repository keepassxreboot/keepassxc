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
#include "DatabaseStats.h"

// Ctor does all the work
DatabaseStats::DatabaseStats(QSharedPointer<Database> db)
    : modified(QFileInfo(db->filePath()).lastModified())
    , m_db(db)
{
    gatherStats(db->rootGroup()->groupsRecursive(true));
}

// Get average password length
int DatabaseStats::averagePwdLength() const
{
    const auto passwords = uniquePasswords + reusedPasswords;
    return passwords == 0 ? 0 : std::round(totalPasswordLength / double(passwords));
}

// Get max number of password reuse (=how many entries
// share the same password)
int DatabaseStats::maxPwdReuse() const
{
    int ret = 0;
    for (const auto& count : m_passwords) {
        ret = std::max(ret, count);
    }
    return ret;
}

// A warning sign is displayed if one of the
// following returns true.
bool DatabaseStats::isAnyExpired() const
{
    return expiredEntries > 0;
}

bool DatabaseStats::areTooManyPwdsReused() const
{
    return reusedPasswords > uniquePasswords / 10;
}

bool DatabaseStats::arePwdsReusedTooOften() const
{
    return maxPwdReuse() > 3;
}

bool DatabaseStats::isAvgPwdTooShort() const
{
    return averagePwdLength() < 10;
}

void DatabaseStats::gatherStats(const QList<Group*>& groups)
{
    auto checker = HealthChecker(m_db);

    for (const auto* group : groups) {
        // Don't count anything in the recycle bin
        if (group->isRecycled()) {
            continue;
        }

        ++groupCount;

        for (const auto* entry : group->entries()) {
            // Don't count anything in the recycle bin
            if (entry->isRecycled()) {
                continue;
            }

            ++entryCount;

            if (entry->isExpired()) {
                ++expiredEntries;
            }

            // Get password statistics
            const auto pwd = entry->password();
            if (!pwd.isEmpty()) {
                if (!m_passwords.contains(pwd)) {
                    ++uniquePasswords;
                } else {
                    ++reusedPasswords;
                }

                if (pwd.size() < PasswordHealth::Length::Short) {
                    ++shortPasswords;
                }

                // Speed up Zxcvbn process by excluding very long passwords and most passphrases
                if (pwd.size() < PasswordHealth::Length::Long
                    && checker.evaluate(entry)->quality() <= PasswordHealth::Quality::Weak) {
                    ++weakPasswords;
                }

                if (entry->excludeFromReports()) {
                    ++excludedEntries;
                }

                totalPasswordLength += pwd.size();
                m_passwords[pwd]++;
            }
        }
    }
}
