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
#include "gui/ApplicationSettingsWidget.h"
#include "gui/CategoryListWidget.h"

SCENARIO_METHOD(FixtureWithDb, "Test application settings default tab order", "[gui]")
{
    WHEN("User opens 'Settings'")
    {
        triggerAction("actionSettings");
        auto* appSettingsWidget = m_mainWindow->findChild<ApplicationSettingsWidget*>();

        THEN("The application settings widget is open and shows expected categories")
        {
            REQUIRE(appSettingsWidget->isVisible());
            REQUIRE(appSettingsWidget->findChild<CategoryListWidget*>("categoryList")->currentCategory() == 0);

            auto categoryWidgets = appSettingsWidget->findChildren<QTabWidget*>();
            REQUIRE_FALSE(categoryWidgets.empty());

            for (auto* w : categoryWidgets) {
                if (w->currentIndex() != 0) {
                    FAIL("Application settings contain QTabWidgets whose default index is not 0");
                }
            }
        }

        escape(appSettingsWidget);
    }
}
