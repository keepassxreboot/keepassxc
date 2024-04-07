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
#include "config-keepassx-tests.h"

#include <QTest>

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"

SCENARIO_METHOD(FixtureWithDb, "Test AutoReload Database", "[gui]")
{
    config()->set(Config::AutoReloadOnChange, false);

    WHEN("User accepts new file in auto-reload")
    {
        MessageBox::setNextAnswer(MessageBox::Yes);

        // Overwrite the current database with a temp db
        REQUIRE(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));
        wait(250);

        REQUIRE_FALSE(m_db == m_dbWidget->database());
        m_db = m_dbWidget->database();

        THEN("The General group contains one entry from the new db")
        {
            REQUIRE(m_db->rootGroup()->findChildByName("General")->entries().size() == 1);
            REQUIRE_FALSE(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));
        }
    }

    WHEN("User rejects new file in auto-reload")
    {
        MessageBox::setNextAnswer(MessageBox::No);

        // Overwrite the current database with a temp db
        REQUIRE(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));
        wait(250);

        REQUIRE(m_db == m_dbWidget->database());

        THEN("The merge did not take place, the General group contains no entry from the new db")
        {
            REQUIRE(m_db->rootGroup()->findChildByName("General")->entries().empty());
            REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));
        }
    }

    WHEN("User accepts a merge of edits into autoreload")
    {
        // Turn on autoload, so we only get one messagebox (for the merge)
        config()->set(Config::AutoReloadOnChange, true);

        // Modify the first entry
        updateEntry(0, "test");

        // This is saying yes to merging the entries
        MessageBox::setNextAnswer(MessageBox::Merge);

        // Overwrite the current database with a temp db
        REQUIRE(m_dbFile.copyFromFile(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx")));
        wait(250);

        REQUIRE_FALSE(m_db == m_dbWidget->database());
        m_db = m_dbWidget->database();

        THEN("The General group contains one merged entry")
        {
            REQUIRE(m_db->rootGroup()->findChildByName("General")->entries().size() == 1);
            REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()).endsWith("*"));
        }
    }
}
