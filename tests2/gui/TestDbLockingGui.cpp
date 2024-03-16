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
#include "../streams.h" // for printable logs on QStrings verification

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/DatabaseTabWidget.h"

// TODO: add a scenario when user has 2 unlocked DBs and locks all DBs.
SCENARIO_METHOD(FixtureWithDb, "Test Lock All Databases", "[gui]")
{
    QString origDbName = m_tabWidget->tabText(0);

    WHEN("User has one unlocked DB and does 'Locks All Databases'")
    {
        triggerAction("actionLockAllDatabases");

        THEN("The DB is locked and DB related actions are disabled")
        {
            REQUIRE(m_dbWidget->isLocked());
            REQUIRE(m_tabWidget->tabText(0) == origDbName + " [Locked]");
            REQUIRE_FALSE(findAction("actionDatabaseMerge")->isEnabled());
            REQUIRE_FALSE(findAction("actionDatabaseSave")->isEnabled());
            // TODO: verify other DB related actions
        }

        WHEN("User again unlocks the DB")
        {
            unlockDbByPassword("a");

            THEN("The DB is unlocked and DB related actions are enabled")
            {
                REQUIRE_FALSE(m_dbWidget->isLocked());
                REQUIRE(m_tabWidget->tabText(0) == origDbName);
                REQUIRE(findAction("actionDatabaseMerge")->isEnabled());
            }
        }
    }
}
