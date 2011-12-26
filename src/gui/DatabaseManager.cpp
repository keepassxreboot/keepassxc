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

#include "DatabaseManager.h"

#include <QtCore/QFileInfo>
#include <QtGui/QTabWidget>

#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass2XmlReader.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/KeyOpenDialog.h"

DatabaseManagerStruct::DatabaseManagerStruct()
    : file(0)
    , dbWidget(0)
    , modified(false)
    , readOnly(false)
{
}

DatabaseManager::DatabaseManager(QTabWidget* tabWidget)
    : m_tabWidget(tabWidget)
    , m_window(tabWidget->window())
{
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabase(int)));
}

void DatabaseManager::newDatabase()
{
    DatabaseManagerStruct dbStruct;
    Database* db = new Database();
    dbStruct.dbWidget = new DatabaseWidget(db, m_tabWidget);

    insertDatabase(db, dbStruct);
}

void DatabaseManager::openDatabase()
{
    QString fileName = fileDialog()->getOpenFileName(m_window, tr("Open database"), QString(),
                                                     tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        openDatabase(fileName);
    }
}

void DatabaseManager::openDatabase(const QString& fileName)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    // TODO error handling
    if (!file->open(QIODevice::ReadWrite)) {
        if (!file->open(QIODevice::ReadOnly)) {
            // can only open read-only
            m_curDbStruct.readOnly = true;
        }
        else {
            // can't open
            return;
        }
    }

    m_curDbStruct.file = file.take();
    m_curDbStruct.fileName = QFileInfo(fileName).absoluteFilePath();

    openDatabaseDialog();
}

void DatabaseManager::openDatabaseDialog()
{
    m_curKeyDialog = new KeyOpenDialog(m_curDbStruct.fileName, m_window);
    connect(m_curKeyDialog, SIGNAL(accepted()), SLOT(openDatabaseRead()));
    connect(m_curKeyDialog, SIGNAL(rejected()), SLOT(openDatabaseCleanup()));
    m_curKeyDialog->setModal(true);
    m_curKeyDialog->show();
}

void DatabaseManager::openDatabaseRead()
{
    m_curDbStruct.file->reset();
    Database* db = m_reader.readDatabase(m_curDbStruct.file, m_curKeyDialog->key());
    delete m_curKeyDialog;
    m_curKeyDialog = 0;

    if (!db) {
        openDatabaseDialog();
    }
    else {
        m_curDbStruct.dbWidget = new DatabaseWidget(db, m_tabWidget);
        insertDatabase(db, m_curDbStruct);
        m_curDbStruct = DatabaseManagerStruct();
    }
}

void DatabaseManager::openDatabaseCleanup()
{
    delete m_curKeyDialog;
    m_curKeyDialog = 0;

    if (m_curDbStruct.file) {
        delete m_curDbStruct.file;
    }
    m_curDbStruct = DatabaseManagerStruct();
}

void DatabaseManager::closeDatabase(Database* db)
{
    Q_ASSERT(db);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);

    if (dbStruct.modified) {
        // TODO message box
    }

    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    m_tabWidget->removeTab(index);
    delete dbStruct.dbWidget;
    delete db;
}

void DatabaseManager::saveDatabase(Database* db)
{
    DatabaseManagerStruct& dbStruct = m_dbList[db];

    // TODO ensure that the data is actually written to disk
    if (dbStruct.file) {
        dbStruct.file->reset();
        m_writer.writeDatabase(dbStruct.file, db);
        dbStruct.file->resize(dbStruct.file->pos());
        dbStruct.file->flush();

        dbStruct.modified = false;
        updateTabName(db);
    }
    else {
        saveDatabaseAs(db);
    }
}

void DatabaseManager::saveDatabaseAs(Database* db)
{
    QString fileName = fileDialog()->getSaveFileName(m_window, tr("Save database as"),
                                                     QString(), tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        DatabaseManagerStruct& dbStruct = m_dbList[db];

        delete dbStruct.file;
        QScopedPointer<QFile> file(new QFile(fileName));
        // TODO error handling
        if (!file->open(QIODevice::ReadWrite)) {
            return;
        }
        dbStruct.file = file.take();
        // TODO ensure that the data is actually written to disk
        m_writer.writeDatabase(dbStruct.file, db);
        dbStruct.file->flush();

        dbStruct.modified = false;
        dbStruct.fileName = QFileInfo(fileName).absoluteFilePath();
        updateTabName(db);
    }
}

void DatabaseManager::closeDatabase(int index)
{
    if (index == -1) {
        index = m_tabWidget->currentIndex();
    }

    closeDatabase(indexDatabase(index));
}

void DatabaseManager::saveDatabase(int index)
{
    if (index == -1) {
        index = m_tabWidget->currentIndex();
    }

    saveDatabase(indexDatabase(index));
}

void DatabaseManager::saveDatabaseAs(int index)
{
    if (index == -1) {
        index = m_tabWidget->currentIndex();
    }
    saveDatabaseAs(indexDatabase(index));
}

void DatabaseManager::updateTabName(Database* db)
{
    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);

    QString tabName;

    if (dbStruct.file) {
        QFileInfo fileInfo(*dbStruct.file);

        if (db->metadata()->name().isEmpty()) {
            tabName = fileInfo.fileName();
        }
        else {
            tabName = db->metadata()->name();
        }

        m_tabWidget->setTabToolTip(index, dbStruct.fileName);
    }
    else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New database");
        }
        else {
            tabName = QString("%1 [%2]").arg(db->metadata()->name(), tr("New database"));
        }
    }

    m_tabWidget->setTabText(index, tabName);
}

int DatabaseManager::databaseIndex(Database* db)
{
    QWidget* dbWidget = m_dbList.value(db).dbWidget;
    return m_tabWidget->indexOf(dbWidget);
}

Database* DatabaseManager::indexDatabase(int index)
{
    QWidget* dbWidget = m_tabWidget->widget(index);

    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().dbWidget == dbWidget) {
            return i.key();
        }
    }

    return 0;
}

void DatabaseManager::insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct)
{
    m_dbList.insert(db, dbStruct);

    m_tabWidget->addTab(dbStruct.dbWidget, "");
    updateTabName(db);
    int index = databaseIndex(db);
    m_tabWidget->setCurrentIndex(index);

    connect(db->metadata(), SIGNAL(nameTextChanged(Database*)), SLOT(updateTabName(Database*)));
}
