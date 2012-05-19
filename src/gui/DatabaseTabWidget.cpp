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

#include "DatabaseTabWidget.h"

#include <QtCore/QFileInfo>
#include <QtGui/QTabWidget>
#include <QtGui/QMessageBox>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/DatabaseOpenDialog.h"
#include "gui/DatabaseWidget.h"
#include "gui/DragTabBar.h"
#include "gui/FileDialog.h"
#include "gui/KeePass1OpenDialog.h"
#include "gui/entry/EntryView.h"
#include "gui/group/GroupView.h"

DatabaseManagerStruct::DatabaseManagerStruct()
    : file(0)
    , dbWidget(0)
    , modified(false)
    , readOnly(false)
{
}

DatabaseTabWidget::DatabaseTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_window(parent->window())
{
    DragTabBar* tabBar = new DragTabBar(this);
    tabBar->setDrawBase(false);
    setTabBar(tabBar);

    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabase(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(emitEntrySelectionChanged()));
}

void DatabaseTabWidget::toggleTabbar() {
    if (count() > 1) {
        if (!tabBar()->isVisible()) {
            tabBar()->show();
        }
    }
    else {
        if (tabBar()->isVisible()) {
            tabBar()->hide();
        }
    }
}

void DatabaseTabWidget::newDatabase()
{
    DatabaseManagerStruct dbStruct;
    Database* db = new Database();
    db->rootGroup()->setName(tr("Root"));
    dbStruct.dbWidget = new DatabaseWidget(db, this);

    insertDatabase(db, dbStruct);

    dbStruct.dbWidget->switchToMasterKeyChange();
}

void DatabaseTabWidget::openDatabase()
{
    QString fileName = fileDialog()->getOpenFileName(m_window, tr("Open database"), QString(),
                                                     tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        openDatabase(fileName);
    }
}

void DatabaseTabWidget::openDatabase(const QString& fileName, const QString& pw,
                                     const QString& keyFile)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    // TODO: error handling
    if (!file->open(QIODevice::ReadWrite)) {
        if (!file->open(QIODevice::ReadOnly)) {
            // can't open
            return;
        }
        else {
            // can only open read-only
            m_curDbStruct.readOnly = true;
        }
    }

    m_curDbStruct.file = file.take();
    m_curDbStruct.fileName = QFileInfo(fileName).absoluteFilePath();

    openDatabaseDialog(pw, keyFile);
}

void DatabaseTabWidget::openDatabaseDialog(const QString& pw, const QString& keyFile)
{
    m_curKeyDialog = new DatabaseOpenDialog(m_curDbStruct.file, m_curDbStruct.fileName, m_window);
    connect(m_curKeyDialog, SIGNAL(accepted()), SLOT(openDatabaseRead()));
    connect(m_curKeyDialog, SIGNAL(rejected()), SLOT(openDatabaseCleanup()));
    m_curKeyDialog->setModal(true);
    m_curKeyDialog->show();

    if (!pw.isNull() || !keyFile.isEmpty()) {
        m_curKeyDialog->enterKey(pw, keyFile);
    }
}

void DatabaseTabWidget::importKeePass1Database()
{
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open KeePass 1 database"), QString(),
            tr("KeePass 1 database") + " (*.kdb);;" + tr("All files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QScopedPointer<QFile> file(new QFile(fileName));
    // TODO: error handling
    if (!file->open(QIODevice::ReadOnly)) {
        return;
    }

    m_curDbStruct.modified = true;

    m_curKeyDialog = new KeePass1OpenDialog(file.take(), fileName, m_window);
    connect(m_curKeyDialog, SIGNAL(accepted()), SLOT(openDatabaseRead()));
    connect(m_curKeyDialog, SIGNAL(rejected()), SLOT(openDatabaseCleanup()));
    m_curKeyDialog->setModal(true);
    m_curKeyDialog->show();
}

void DatabaseTabWidget::openDatabaseRead()
{
    Database* db = m_curKeyDialog->database();

    delete m_curKeyDialog;
    m_curKeyDialog = 0;

    m_curDbStruct.dbWidget = new DatabaseWidget(db, this);
    insertDatabase(db, m_curDbStruct);
    m_curDbStruct = DatabaseManagerStruct();
}

void DatabaseTabWidget::openDatabaseCleanup()
{
    delete m_curKeyDialog;
    m_curKeyDialog = 0;

    if (m_curDbStruct.file) {
        delete m_curDbStruct.file;
    }
    m_curDbStruct = DatabaseManagerStruct();
}

void DatabaseTabWidget::emitEntrySelectionChanged()
{
    DatabaseWidget* dbWidget = currentDatabaseWidget();

    bool isSingleEntrySelected = false;
    if (dbWidget) {
        isSingleEntrySelected = dbWidget->entryView()->isSingleEntrySelected();
    }

    Q_EMIT entrySelectionChanged(isSingleEntrySelected);
}

bool DatabaseTabWidget::closeDatabase(Database* db)
{
    Q_ASSERT(db);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);
    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    QString dbName = tabText(index);
    if (dbName.right(1) == "*") {
        dbName.chop(1);
    }
    if (dbStruct.dbWidget->currentMode() == DatabaseWidget::EditMode && db->hasKey()) {
        QMessageBox::StandardButton result =
            QMessageBox::question(
            this, tr("Close?"),
            tr("\"%1\" is in edit mode.\nClose anyway?").arg(dbName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (result == QMessageBox::No) {
            return false;
        }
    }
    if (dbStruct.modified) {
        QMessageBox::StandardButton result =
            QMessageBox::question(
            this, tr("Save changes?"),
            tr("\"%1\" was modified.\nSave changes?").arg(dbName),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
        if (result == QMessageBox::Yes) {
            saveDatabase(db);
        }
        else if (result == QMessageBox::Cancel) {
            return false;
        }
    }

    removeTab(index);
    toggleTabbar();
    m_dbList.remove(db);
    delete dbStruct.file;
    delete dbStruct.dbWidget;
    delete db;
    return true;
}

bool DatabaseTabWidget::closeAllDatabases() {
    while (!m_dbList.isEmpty()) {
        if (!closeDatabase()) {
            return false;
        }
    }
    return true;
}

void DatabaseTabWidget::saveDatabase(Database* db)
{
    DatabaseManagerStruct& dbStruct = m_dbList[db];

    // TODO: ensure that the data is actually written to disk
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

void DatabaseTabWidget::saveDatabaseAs(Database* db)
{
    DatabaseManagerStruct& dbStruct = m_dbList[db];
    QString oldFileName;
    if (dbStruct.file) {
        oldFileName = dbStruct.fileName;
    }
    QString fileName = fileDialog()->getSaveFileName(m_window, tr("Save database as"),
                                                     oldFileName, tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        QFile* oldFile = dbStruct.file;
        QScopedPointer<QFile> file(new QFile(fileName));
        // TODO: error handling
        if (!file->open(QIODevice::ReadWrite)) {
            return;
        }
        dbStruct.file = file.take();
        // TODO: ensure that the data is actually written to disk
        m_writer.writeDatabase(dbStruct.file, db);
        dbStruct.file->flush();
        delete oldFile;

        dbStruct.modified = false;
        dbStruct.fileName = QFileInfo(fileName).absoluteFilePath();
        updateTabName(db);
    }
}

bool DatabaseTabWidget::closeDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    setCurrentIndex(index);

    return closeDatabase(indexDatabase(index));
}

void DatabaseTabWidget::closeDatabaseFromSender()
{
    Q_ASSERT(sender());
    DatabaseWidget* dbWidget = static_cast<DatabaseWidget*>(sender());
    Database* db = databaseFromDatabaseWidget(dbWidget);
    int index = databaseIndex(db);
    setCurrentIndex(index);
    closeDatabase(db);
}

void DatabaseTabWidget::saveDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    saveDatabase(indexDatabase(index));
}

void DatabaseTabWidget::saveDatabaseAs(int index)
{
    if (index == -1) {
        index = currentIndex();
    }
    saveDatabaseAs(indexDatabase(index));
}

void DatabaseTabWidget::changeMasterKey()
{
    currentDatabaseWidget()->switchToMasterKeyChange();
}

void DatabaseTabWidget::changeDatabaseSettings()
{
    currentDatabaseWidget()->switchToDatabaseSettings();
}

void DatabaseTabWidget::createEntry()
{
    currentDatabaseWidget()->createEntry();
}

void DatabaseTabWidget::cloneEntry()
{
    currentDatabaseWidget()->cloneEntry();
}

void DatabaseTabWidget::editEntry()
{
    currentDatabaseWidget()->switchToEntryEdit();
}

void DatabaseTabWidget::deleteEntry()
{
    currentDatabaseWidget()->deleteEntry();
}

void DatabaseTabWidget::createGroup()
{
    currentDatabaseWidget()->createGroup();
}

void DatabaseTabWidget::editGroup()
{
    currentDatabaseWidget()->switchToGroupEdit();
}

void DatabaseTabWidget::deleteGroup()
{
    currentDatabaseWidget()->deleteGroup();
}

void DatabaseTabWidget::toggleSearch()
{
    currentDatabaseWidget()->toggleSearch();
}

void DatabaseTabWidget::updateTabName(Database* db)
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

        setTabToolTip(index, dbStruct.fileName);
    }
    else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New database");
        }
        else {
            tabName = QString("%1 [%2]").arg(db->metadata()->name(), tr("New database"));
        }
    }
    if (dbStruct.modified) {
        tabName.append("*");
    }
    setTabText(index, tabName);
    Q_EMIT tabNameChanged();
}

void DatabaseTabWidget::updateTabNameFromSender()
{
    Q_ASSERT(qobject_cast<Database*>(sender()));

    updateTabName(static_cast<Database*>(sender()));
}

int DatabaseTabWidget::databaseIndex(Database* db)
{
    QWidget* dbWidget = m_dbList.value(db).dbWidget;
    return indexOf(dbWidget);
}

Database* DatabaseTabWidget::indexDatabase(int index)
{
    QWidget* dbWidget = widget(index);

    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().dbWidget == dbWidget) {
            return i.key();
        }
    }

    return 0;
}

Database* DatabaseTabWidget::databaseFromDatabaseWidget(DatabaseWidget* dbWidget)
{
    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().dbWidget == dbWidget) {
            return i.key();
        }
    }

    return 0;
}

void DatabaseTabWidget::insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct)
{
    m_dbList.insert(db, dbStruct);

    addTab(dbStruct.dbWidget, "");
    toggleTabbar();
    updateTabName(db);
    int index = databaseIndex(db);
    setCurrentIndex(index);

    connect(db, SIGNAL(nameTextChanged()), SLOT(updateTabNameFromSender()));
    connect(dbStruct.dbWidget->entryView(), SIGNAL(entrySelectionChanged()),
            SLOT(emitEntrySelectionChanged()));
    connect(dbStruct.dbWidget, SIGNAL(closeRequest()), SLOT(closeDatabaseFromSender()));
    connect(db, SIGNAL(modified()), SLOT(modified()));
    connect(dbStruct.dbWidget, SIGNAL(currentModeChanged(DatabaseWidget::Mode)),
            SIGNAL(currentWidgetModeChanged(DatabaseWidget::Mode)));
}

DatabaseWidget* DatabaseTabWidget::currentDatabaseWidget()
{
    Database* db = indexDatabase(currentIndex());
    if (db) {
        return m_dbList[db].dbWidget;
    }
    else {
        return 0;
    }
}

void DatabaseTabWidget::modified()
{
    Q_ASSERT(qobject_cast<Database*>(sender()));

    Database* db = static_cast<Database*>(sender());
    DatabaseManagerStruct& dbStruct = m_dbList[db];
    if (!dbStruct.modified) {
        dbStruct.modified = true;
        updateTabName(db);
    }
}
