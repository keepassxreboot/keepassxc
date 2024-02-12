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

SCENARIO_METHOD(FixtureWithDb, "Test Save", "[gui]")
{
    WHEN("User changes the DB and saves it")
    {
        m_db->metadata()->setName("testSave");
        wait(250);

        THEN("The DB is saved")
        {
            saveAndCheckDatabase();
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test Save Backup", "[gui]")
{
    QFileInfo dbFileInfo(m_dbFilePath);
    QDateTime dbFileLastModified = dbFileInfo.lastModified();
    QString tmpFileName = newTempFileName();

    WHEN("User changes the DB and saves the current DB as a backup")
    {
        m_db->metadata()->setName("testSaveBackup");

        fileDialog()->setNextFileName(tmpFileName);
        triggerAction("actionDatabaseSaveBackup");

        THEN("The saved DB is as expected")
        {
            REQUIRE_FALSE(m_dbWidget->isLocked());
            REQUIRE(m_tabWidget->tabText(m_tabWidget->currentIndex()) == QString("testSaveBackup"));
            checkDatabase(tmpFileName);

            dbFileInfo.refresh();
            REQUIRE(dbFileInfo.lastModified() == dbFileLastModified);
        }
    }
}

SCENARIO_METHOD(FixtureWithDb, "Test Save Backup Path", "[gui]")
{
    QFileInfo dbFileInfo(m_dbFilePath);

    /**
     * Tests that the backupFilePathPattern config entry is respected. We do not test patterns like {TIME} etc here
     * as this is done in a separate test case. We do however check {DB_FILENAME} as this is a feature of the
     * performBackup() function.
     */
    WHEN("User changes the DB and saves the current DB as a backup")
    {
        struct TestData {
            QString backupFilePathPattern;
            QString expectedBackupFile;
        };

        QString tmpFileName = newTempFileName();

        auto data = GENERATE_REF(
            // Absolute backup path
            TestData{tmpFileName, tmpFileName},
            // Relative backup path (implicit)
            TestData{"other_dir/test.old.kdbx", "other_dir/test.old.kdbx"},
            // Relative backup path (explicit)
            TestData{"./other_dir2/test2.old.kdbx", "other_dir2/test2.old.kdbx"},
            // Path with placeholders
            TestData{"{DB_FILENAME}.old.kdbx", "KeePassXC.old.kdbx"},
            // empty path should be replaced with default pattern
            TestData{"", config()->getDefault(Config::BackupFilePathPattern).toString()},
            // {DB_FILENAME} should be replaced with database filename
            TestData { "{DB_FILENAME}_.old.kdbx", "{DB_FILENAME}_.old.kdbx" }
        );

        // Enable automatic backups
        config()->set(Config::BackupBeforeSave, true);
        config()->set(Config::BackupFilePathPattern, data.backupFilePathPattern);

        // Replace placeholders and resolve relative paths. This cannot be done in the _data() function as the
        // db path/filename is not known yet
        if (!QDir::isAbsolutePath(data.expectedBackupFile)) {
            data.expectedBackupFile = QDir(dbFileInfo.absolutePath()).absoluteFilePath(data.expectedBackupFile);
        }
        data.expectedBackupFile.replace("{DB_FILENAME}", dbFileInfo.completeBaseName());

        // Save a modified database
        auto prevName = m_db->metadata()->name();
        m_db->metadata()->setName("testBackupPathPattern");
        wait(250);

        THEN("The saved DB is as expected")
        {
            saveAndCheckDatabase();

            // Test that the backup file has the previous database name
            checkDatabase(data.expectedBackupFile, prevName);

            // Clean up
            QFile(data.expectedBackupFile).remove();
        }
    }
}
