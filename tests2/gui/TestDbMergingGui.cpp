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

#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include "FixtureWithDb.h"
#include "catch2/catch_all.hpp"
#include "gui/DatabaseTabWidget.h"
#include "gui/FileDialog.h"

SCENARIO_METHOD(FixtureWithDb, "Test Merging from a Database", "[gui]")
{
    // It is safe to ignore the warning this line produces
    QSignalSpy dbMergeSpy(m_dbWidget.data(), SIGNAL(databaseMerged(QSharedPointer<Database>)));
    QApplication::processEvents();

    WHEN("User opens a file to merge from and enters a correct password")
    {
        fileDialog()->setNextFileName(QString(KEEPASSX_TEST_DATA_DIR).append("/MergeDatabase.kdbx"));
        triggerAction("actionDatabaseMerge");

        auto* editPasswordMerge = QApplication::focusWidget();
        REQUIRE(editPasswordMerge->objectName() == QString("passwordEdit"));
        REQUIRE(editPasswordMerge->isVisible());

        QTest::keyClicks(editPasswordMerge, "a");
        QTest::keyClick(editPasswordMerge, Qt::Key_Enter);
        wait(250);

        THEN("The current DB was merged successfully")
        {
            REQUIRE(dbMergeSpy.count() == 1);
            REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()).contains("*"));

            m_db = m_tabWidget->currentDatabaseWidget()->database();

            // there are seven child groups of the root group
            REQUIRE(m_db->rootGroup()->children().size() == 7);

            // the merged group should contain an entry
            REQUIRE(m_db->rootGroup()->children().at(6)->entries().size() == 1);

            // the General group contains one entry merged from the other db
            REQUIRE(m_db->rootGroup()->findChildByName("General")->entries().size() == 1);
        }
    }
}
