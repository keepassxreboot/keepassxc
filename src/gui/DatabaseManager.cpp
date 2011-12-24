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
#include <QtGui/QFileDialog>
#include <QtGui/QTabWidget>

#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass2XmlReader.h"
#include "gui/DatabaseWidget.h"
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
    QString fileName = QFileDialog::getOpenFileName(m_window, tr("Open database"), QString(),
                                                    tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        openDatabase(fileName);
    }
}

void DatabaseManager::openDatabase(const QString& fileName)
{
    DatabaseManagerStruct dbStruct;

    QScopedPointer<QFile> file(new QFile(fileName));
    // TODO error handling
    if (!file->open(QIODevice::ReadWrite)) {
        if (!file->open(QIODevice::ReadOnly)) {
            // can only open read-only
            dbStruct.readOnly = true;
        }
        else {
            // can't open
            return;
        }
    }

    Database* db;

    do {
        QScopedPointer<KeyOpenDialog> keyDialog(new KeyOpenDialog(m_window));
        if (keyDialog->exec() == QDialog::Rejected) {
            return;
        }

        file->reset();
        db = m_reader.readDatabase(file.data(), keyDialog->key());
    } while (!db);

    dbStruct.file = file.take();
    dbStruct.dbWidget = new DatabaseWidget(db, m_tabWidget);

    insertDatabase(db, dbStruct);
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
    QString fileName = QFileDialog::getSaveFileName(m_window, tr("Save database as"),
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

        QString filename = QFileInfo(*dbStruct.file).completeBaseName();

        if (db->metadata()->name().isEmpty()) {
            tabName = filename;
        }
        else {
            tabName = QString("%1 [%2]").arg(db->metadata()->name().arg(filename));
        }
    }
    else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New database");
        }
        else {
            tabName = QString("%1 [%2]").arg(db->metadata()->name().arg(tr("New database")));
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

    connect(db->metadata(), SIGNAL(nameTextChanged(Database*)), SLOT(updateTabName(Database*)));
}
