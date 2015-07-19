/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include <QAbstractItemModel>
#include <QObject>
#include <QTemporaryFile>

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
class QAbstractItemView;
class MainWindow;

class TestGui : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testOpenDatabase();
    void testTabs();
    void testEditEntry();
    void testAddEntry();
    void testSearch();
    void testDeleteEntry();
    void testCloneEntry();
    void testDragAndDropEntry();
    void testDragAndDropGroup();
    void testSaveAs();
    void testSave();
    void testDatabaseSettings();
    void testKeePass1Import();
    void testDatabaseLocking();
    void cleanupTestCase();

private:
    void checkDatabase();
    void triggerAction(const QString& name);
    void dragAndDropGroup(const QModelIndex& sourceIndex, const QModelIndex& targetIndex, int row,
                          bool expectedResult, const QString& expectedParentName, int expectedPos);
    void clickIndex(const QModelIndex& index, QAbstractItemView* view, Qt::MouseButton button,
                    Qt::KeyboardModifiers stateKey = 0);

    MainWindow* m_mainWindow;
    DatabaseTabWidget* m_tabWidget;
    DatabaseWidget* m_dbWidget;
    QTemporaryFile m_orgDbFile;
    QString m_orgDbFileName;
    QString m_tmpFileName;
    Database* m_db;
};

#endif // KEEPASSX_TESTGUI_H
