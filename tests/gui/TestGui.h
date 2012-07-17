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

#include <QtCore/QObject>
#include <QtCore/QTemporaryFile>

class Database;
class DatabaseTabWidget;
class DatabaseWidget;
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
    void testSaveAs();
    void testSave();
    void testDatabaseSettings();
    void testKeePass1Import();
    void cleanupTestCase();

private:
    bool checkDatabase();

    MainWindow* m_mainWindow;
    DatabaseTabWidget* m_tabWidget;
    DatabaseWidget* m_dbWidget;
    QTemporaryFile m_tmpFile;
    Database* m_db;
};

#endif // KEEPASSX_TESTGUI_H
