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
#include "gui/CategoryListWidget.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"

SCENARIO_METHOD(FixtureWithDb, "Test database settings default tab order", "[gui]")
{
    WHEN("User has one unlocked DB and opens 'Database Settings'")
    {
        triggerAction("actionDatabaseSettings");
        auto* dbSettingsWidget = m_mainWindow->findChild<DatabaseSettingsDialog*>();

        THEN("The DB settings widget is open and shows expected categories")
        {
            REQUIRE(dbSettingsWidget->isVisible());
            REQUIRE(dbSettingsWidget->findChild<CategoryListWidget*>("categoryList")->currentCategory() == 0);

            auto categoryWidgets = dbSettingsWidget->findChildren<QTabWidget*>();
            REQUIRE_FALSE(categoryWidgets.empty());

            for (auto* w : categoryWidgets) {
                if (w->currentIndex() != 0 && w->objectName() != "encryptionSettingsTabWidget") {
                    FAIL("Database settings contain QTabWidgets whose default index is not 0");
                }
            }
        }

        escape(dbSettingsWidget);
    }
}
