/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTGUI_H
#define KEEPASSX_TESTGUI_H

#include "GuiTestBase.h"
#include "gui/MainWindow.h"
#include "util/TemporaryFile.h"

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class QAbstractItemView;

class TestGui : public GuiTestBase
{
    Q_OBJECT

private slots:
    void testSettingsDefaultTabOrder();
    void testCreateDatabase();
    void testMergeDatabase();
    void testAutoreloadDatabase();
    void testTabs();
    void testEditEntry();
    void testSearchEditEntry();
    void testAddEntry();
    void testPasswordEntryEntropy();
    void testPasswordEntryEntropy_data();
    void testDicewareEntryEntropy();
    void testSearch();
    void testDeleteEntry();
    void testCloneEntry();
    void testEntryPlaceholders();
    void testDragAndDropEntry();
    void testDragAndDropGroup();
    void testSaveAs();
    void testSaveBackup();
    void testSave();
    void testSaveBackupPath();
    void testSaveBackupPath_data();
    void testDatabaseSettings();
    void testKeePass1Import();
    void testDatabaseLocking();
    void testDragAndDropKdbxFiles();
    void testSortGroups();
    void testAutoType();
    void testTrayRestoreHide();

private:
    void addCannedEntries();
    void checkDatabase(const QString& filePath, const QString& expectedDbName);
    void checkDatabase(const QString& filePath = {});
    void dragAndDropGroup(const QModelIndex& sourceIndex,
                          const QModelIndex& targetIndex,
                          int row,
                          bool expectedResult,
                          const QString& expectedParentName,
                          int expectedPos);

    void checkSaveDatabase();
    void checkStatusBarText(const QString& textFragment);
};

#endif // KEEPASSX_TESTGUI_H
