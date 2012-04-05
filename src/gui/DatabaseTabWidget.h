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

#ifndef KEEPASSX_DATABASETABWIDGET_H
#define KEEPASSX_DATABASETABWIDGET_H

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtGui/QTabWidget>

#include "format/KeePass2Writer.h"

class DatabaseWidget;
class DatabaseOpenDialog;
class QFile;
class QTabWidget;

struct DatabaseManagerStruct
{
    DatabaseManagerStruct();

    QFile* file;
    DatabaseWidget* dbWidget;
    QString fileName;
    bool modified;
    bool readOnly;
};

class DatabaseTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit DatabaseTabWidget(QWidget* parent);
    void openDatabase(const QString& fileName);
    DatabaseWidget* currentDatabaseWidget();

public Q_SLOTS:
    void newDatabase();
    void openDatabase();
    void saveDatabase(int index = -1);
    void saveDatabaseAs(int index = -1);
    void closeDatabase(int index = -1);
    void changeMasterKey();
    void createEntry();
    void editEntry();
    void createGroup();
    void editGroup();

Q_SIGNALS:
    void entrySelectionChanged(bool singleEntrySelected);

private Q_SLOTS:
    void updateTabName(Database* db);
    void openDatabaseDialog();
    void openDatabaseRead();
    void openDatabaseCleanup();
    void emitEntrySelectionChanged();

private:
    void saveDatabase(Database* db);
    void saveDatabaseAs(Database* db);
    void closeDatabase(Database* db);
    int databaseIndex(Database* db);
    Database* indexDatabase(int index);
    void insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct);

    QWidget* m_window;
    KeePass2Writer m_writer;
    QHash<Database*, DatabaseManagerStruct> m_dbList;
    DatabaseManagerStruct m_curDbStruct;
    DatabaseOpenDialog* m_curKeyDialog;
};

#endif // KEEPASSX_DATABASETABWIDGET_H
