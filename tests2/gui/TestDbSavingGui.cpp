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
#include <QFileInfo>

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"

SCENARIO_METHOD(FixtureWithDb, "Test Save As...", "[gui]")
{
    QFileInfo dbFileInfo(m_dbFilePath);
    QDateTime dbFileLastModified = dbFileInfo.lastModified();
    QString tmpFileName = newTempFileName();

    WHEN("User changes the DB and saves the current DB as a new file")
    {
        m_db->metadata()->setName("testSaveAs");

        fileDialog()->setNextFileName(tmpFileName);
        triggerAction("actionDatabaseSaveAs");

        THEN("The saved DB is as expected")
        {
            REQUIRE_FALSE(m_dbWidget->isLocked());
            REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()) == QString("testSaveAs"));
            checkDatabase(tmpFileName);

            dbFileInfo.refresh();
            REQUIRE(dbFileInfo.lastModified() == dbFileLastModified);
        }
    }
}
