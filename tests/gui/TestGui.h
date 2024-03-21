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

#include "gui/MainWindow.h"
#include "util/TemporaryFile.h"

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class QAbstractItemView;

class TestGui : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

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
    void testTotp();
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
    void testDatabaseLocking();
    void testDragAndDropKdbxFiles();
    void testSortGroups();
    void testAutoType();
    void testTrayRestoreHide();
    void testShortcutConfig();

private:
    void addCannedEntries();
    void checkDatabase(const QString& filePath, const QString& expectedDbName);
    void checkDatabase(const QString& filePath = {});
    void triggerAction(const QString& name);
    void dragAndDropGroup(const QModelIndex& sourceIndex,
                          const QModelIndex& targetIndex,
                          int row,
                          bool expectedResult,
                          const QString& expectedParentName,
                          int expectedPos);
    void clickIndex(const QModelIndex& index,
                    QAbstractItemView* view,
                    Qt::MouseButton button,
                    Qt::KeyboardModifiers stateKey = 0);
    void checkSaveDatabase();
    void checkStatusBarText(const QString& textFragment);

    QScopedPointer<MainWindow> m_mainWindow;
    QPointer<QLabel> m_statusBarLabel;
    QPointer<DatabaseTabWidget> m_tabWidget;
    QPointer<DatabaseWidget> m_dbWidget;
    QSharedPointer<Database> m_db;
    TemporaryFile m_dbFile;
    QString m_dbFileName;
    QString m_dbFilePath;
};

#endif // KEEPASSX_TESTGUI_H
