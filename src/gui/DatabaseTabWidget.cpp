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

#include <QFileInfo>
#include <QTabWidget>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/qsavefile.h"
#include "gui/DatabaseWidget.h"
#include "gui/DragTabBar.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/entry/EntryView.h"
#include "gui/group/GroupView.h"

DatabaseManagerStruct::DatabaseManagerStruct()
    : dbWidget(Q_NULLPTR)
    , saveToFilename(false)
    , modified(false)
    , readOnly(false)
{
}


const int DatabaseTabWidget::LastDatabasesCount = 5;

DatabaseTabWidget::DatabaseTabWidget(QWidget* parent)
    : QTabWidget(parent),
      m_fileWatcher(new QFileSystemWatcher(this))
{
    DragTabBar* tabBar = new DragTabBar(this);
    tabBar->setDrawBase(false);
    setTabBar(tabBar);

    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabase(int)));
    connect(autoType(), SIGNAL(globalShortcutTriggered()), SLOT(performGlobalAutoType()));
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)), SLOT(fileChanged(QString)));
}

DatabaseTabWidget::~DatabaseTabWidget()
{
    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        deleteDatabase(i.key());
    }
}

void DatabaseTabWidget::toggleTabbar()
{
    tabBar()->setVisible(count() > 1);
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
    QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open database"), QString(),
                                                     filter);
    if (!fileName.isEmpty()) {
        openDatabase(fileName);
    }
}

void DatabaseTabWidget::openDatabase(const QString& fileName, const QString& pw,
                                     const QString& keyFile, const CompositeKey& key, int index)
{
    QFileInfo fileInfo(fileName);
    QString canonicalFilePath = fileInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty()) {
        MessageBox::warning(this, tr("Warning"), tr("File not found!"));
        return;
    }


    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().canonicalFilePath == canonicalFilePath) {
            setCurrentIndex(databaseIndex(i.key()));
            return;
        }
    }

    DatabaseManagerStruct dbStruct;

    // test if we can read/write or read the file
    QFile file(fileName);
    // TODO: error handling
    if (!file.open(QIODevice::ReadWrite)) {
        if (!file.open(QIODevice::ReadOnly)) {
            // can't open
            // TODO: error message
            return;
        }
        else {
            // can only open read-only
            dbStruct.readOnly = true;
        }
    }
    file.close();

    Database* db = new Database();
    dbStruct.dbWidget = new DatabaseWidget(db, this);
    dbStruct.saveToFilename = !dbStruct.readOnly;

    dbStruct.filePath = fileInfo.absoluteFilePath();
    dbStruct.canonicalFilePath = canonicalFilePath;
    dbStruct.fileName = fileInfo.fileName();
    dbStruct.lastModified = fileInfo.lastModified();

    insertDatabase(db, dbStruct, index);
    m_fileWatcher->addPath(dbStruct.filePath);

    updateRecentDatabases(dbStruct.filePath);

    if (!key.isEmpty()) {
        dbStruct.dbWidget->switchToOpenDatabase(dbStruct.filePath, key);
    }
    else if (!pw.isNull() || !keyFile.isEmpty()) {
        dbStruct.dbWidget->switchToOpenDatabase(dbStruct.filePath, pw, keyFile);
    }
    else {
        dbStruct.dbWidget->switchToOpenDatabase(dbStruct.filePath);
    }
}

void DatabaseTabWidget::importKeePass1Database()
{
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open KeePass 1 database"), QString(),
            tr("KeePass 1 database") + " (*.kdb);;" + tr("All files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    Database* db = new Database();
    DatabaseManagerStruct dbStruct;
    dbStruct.dbWidget = new DatabaseWidget(db, this);
    dbStruct.modified = true;

    insertDatabase(db, dbStruct);

    dbStruct.dbWidget->switchToImportKeepass1(fileName);
}

void DatabaseTabWidget::fileChanged(const QString &fileName)
{
    const bool wasEmpty = m_changedFiles.isEmpty();
    m_changedFiles.insert(fileName);
    bool found = false;
    Q_FOREACH (QString f, m_fileWatcher->files()) {
        if (f == fileName) {
            found = true;
            break;
        }
    }
    if (!found) m_fileWatcher->addPath(fileName);
    if (wasEmpty && !m_changedFiles.isEmpty())
        QTimer::singleShot(200, this, SLOT(checkReloadDatabases()));
}

void DatabaseTabWidget::expectFileChange(const DatabaseManagerStruct& dbStruct)
{
    if (dbStruct.filePath.isEmpty())
        return;
    m_expectedFileChanges.insert(dbStruct.filePath);
}

void DatabaseTabWidget::unexpectFileChange(DatabaseManagerStruct& dbStruct)
{
    if (dbStruct.filePath.isEmpty())
        return;
    m_expectedFileChanges.remove(dbStruct.filePath);
    dbStruct.lastModified = QFileInfo(dbStruct.filePath).lastModified();
}

void DatabaseTabWidget::checkReloadDatabases()
{
    QSet<QString> changedFiles;

    changedFiles = m_changedFiles.subtract(m_expectedFileChanges);
    m_changedFiles.clear();

    if (changedFiles.isEmpty())
        return;

    Q_FOREACH (DatabaseManagerStruct dbStruct, m_dbList) {
        QString filePath = dbStruct.filePath;
        Database * db = dbStruct.dbWidget->database();

        if (!changedFiles.contains(filePath))
            continue;

        QFileInfo fi(filePath);
        QDateTime lastModified = fi.lastModified();
        if (dbStruct.lastModified == lastModified)
            continue;

        DatabaseWidget::Mode mode = dbStruct.dbWidget->currentMode();
        if (mode == DatabaseWidget::None || mode == DatabaseWidget::LockedMode || !db->hasKey())
            continue;

        ReloadBehavior reloadBehavior = ReloadBehavior(config()->get("ReloadBehavior").toInt());
        if (   (reloadBehavior == AlwaysAsk)
            || (reloadBehavior == ReloadUnmodified && (mode == DatabaseWidget::EditMode
                || mode == DatabaseWidget::OpenMode))
            || (reloadBehavior == ReloadUnmodified && dbStruct.modified)) {
            int res = QMessageBox::warning(this, fi.exists() ? tr("Database file changed") : tr("Database file removed"),
                                           tr("Do you want to discard your changes and reload?"),
                                           QMessageBox::Yes|QMessageBox::No);
            if (res == QMessageBox::No)
                continue;
        }

        if (fi.exists()) {
            //Ignore/cancel all edits
            dbStruct.dbWidget->switchToView(false);
            dbStruct.modified = false;

            //Save current group/entry
            Uuid currentGroup;
            if (Group* group = dbStruct.dbWidget->currentGroup())
                currentGroup = group->uuid();
            Uuid currentEntry;
            if (Entry* entry = dbStruct.dbWidget->entryView()->currentEntry())
                currentEntry = entry->uuid();
            QString searchText = dbStruct.dbWidget->searchText();
            bool caseSensitive = dbStruct.dbWidget->caseSensitiveSearch();
            bool allGroups =     dbStruct.dbWidget->isAllGroupsSearch();

            //Reload updated db
            CompositeKey key = db->key();
            int tabPos = databaseIndex(db);
            closeDatabase(db);
            openDatabase(filePath, QString(), QString(), key, tabPos);

            //Restore current group/entry
            dbStruct = indexDatabaseManagerStruct(count() - 1);
            if (dbStruct.dbWidget && dbStruct.dbWidget->currentMode() == DatabaseWidget::ViewMode) {
                Database * db = dbStruct.dbWidget->database();
                if (!currentGroup.isNull())
                    if (Group* group = db->resolveGroup(currentGroup))
                        dbStruct.dbWidget->groupView()->setCurrentGroup(group);
                if (!searchText.isEmpty())
                    dbStruct.dbWidget->search(searchText, caseSensitive, allGroups);
                if (!currentEntry.isNull())
                    if (Entry* entry = db->resolveEntry(currentEntry))
                        dbStruct.dbWidget->entryView()->setCurrentEntry(entry);
            }
        } else {
            //Ignore/cancel all edits
            dbStruct.dbWidget->switchToView(false);
            dbStruct.modified = false;

            //Close database
            closeDatabase(dbStruct.dbWidget->database());
        }
    }
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
    if ((dbStruct.dbWidget->currentMode() == DatabaseWidget::EditMode ||
         dbStruct.dbWidget->currentMode() == DatabaseWidget::OpenMode) && db->hasKey()) {
        QMessageBox::StandardButton result =
            MessageBox::question(
            this, tr("Close?"),
            tr("\"%1\" is in edit mode.\nClose anyway?").arg(dbName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (result == QMessageBox::No) {
            return false;
        }
    }
    if (dbStruct.modified) {
        if (config()->get("AutoSaveOnExit").toBool()) {
            saveDatabase(db);
        }
        else {
            QMessageBox::StandardButton result =
                MessageBox::question(
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
    }

    deleteDatabase(db);

    return true;
}

void DatabaseTabWidget::deleteDatabase(Database* db)
{
    const DatabaseManagerStruct dbStruct = m_dbList.value(db);
    bool emitDatabaseWithFileClosed = dbStruct.saveToFilename;
    QString filePath = dbStruct.filePath;

    int index = databaseIndex(db);

    m_fileWatcher->removePath(dbStruct.filePath);
    removeTab(index);
    toggleTabbar();
    m_dbList.remove(db);
    delete dbStruct.dbWidget;
    delete db;

    if (emitDatabaseWithFileClosed) {
        Q_EMIT databaseWithFileClosed(filePath);
    }
}

bool DatabaseTabWidget::closeAllDatabases()
{
    QStringList reloadDatabases;
    if (config()->get("AutoReopenLastDatabases", false).toBool()) {
        for (int i = 0; i < count(); i ++)
            reloadDatabases << indexDatabaseManagerStruct(i).filePath;
    }
    config()->set("LastOpenDatabases", reloadDatabases);

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

    if (dbStruct.saveToFilename) {
        bool result = false;

        expectFileChange(dbStruct);

        QSaveFile saveFile(dbStruct.filePath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            m_writer.writeDatabase(&saveFile, db);
            result = saveFile.commit();
        }

        unexpectFileChange(dbStruct);

        if (result) {
            dbStruct.modified = false;
            updateTabName(db);
        }
        else {
            MessageBox::critical(this, tr("Error"), tr("Writing the database failed.") + "\n\n"
                                 + saveFile.errorString());
        }
    }
    else {
        saveDatabaseAs(db);
    }
}

void DatabaseTabWidget::saveDatabaseAs(Database* db)
{
    DatabaseManagerStruct& dbStruct = m_dbList[db];
    QString oldFilePath;
    if (dbStruct.saveToFilename) {
        oldFilePath = dbStruct.filePath;
    }
    QString fileName = fileDialog()->getSaveFileName(this, tr("Save database as"),
                                                     oldFilePath, tr("KeePass 2 Database").append(" (*.kdbx)"));
    if (!fileName.isEmpty()) {
        bool result = false;

        QSaveFile saveFile(fileName);
        if (saveFile.open(QIODevice::WriteOnly)) {
            m_writer.writeDatabase(&saveFile, db);
            result = saveFile.commit();
        }

        if (result) {
            m_fileWatcher->removePath(oldFilePath);
            dbStruct.modified = false;
            dbStruct.saveToFilename = true;
            QFileInfo fileInfo(fileName);
            dbStruct.filePath = fileInfo.absoluteFilePath();
            dbStruct.canonicalFilePath = fileInfo.canonicalFilePath();
            dbStruct.fileName = fileInfo.fileName();
            dbStruct.lastModified = fileInfo.lastModified();
            dbStruct.dbWidget->updateFilename(dbStruct.filePath);
            updateTabName(db);
            updateRecentDatabases(dbStruct.filePath);
            m_fileWatcher->addPath(dbStruct.filePath);
        }
        else {
            MessageBox::critical(this, tr("Error"), tr("Writing the database failed.") + "\n\n"
                                 + saveFile.errorString());
        }
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

bool DatabaseTabWidget::readOnly(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return indexDatabaseManagerStruct(index).readOnly;
}

void DatabaseTabWidget::updateTabName(Database* db)
{
    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);

    QString tabName;

    if (dbStruct.saveToFilename) {
        if (db->metadata()->name().isEmpty()) {
            tabName = dbStruct.fileName;
        }
        else {
            tabName = db->metadata()->name();
        }

        setTabToolTip(index, dbStruct.filePath);
    }
    else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New database");
        }
        else {
            tabName = QString("%1 [%2]").arg(db->metadata()->name(), tr("New database"));
        }
    }

    if (dbStruct.dbWidget->currentMode() == DatabaseWidget::LockedMode) {
        tabName.append(QString(" [%1]").arg(tr("locked")));
    }

    if (dbStruct.modified) {
        tabName.append("*");
    }

    setTabText(index, tabName);
    Q_EMIT tabNameChanged();
}

void DatabaseTabWidget::updateTabNameFromDbSender()
{
    Q_ASSERT(qobject_cast<Database*>(sender()));

    updateTabName(static_cast<Database*>(sender()));
}

void DatabaseTabWidget::updateTabNameFromDbWidgetSender()
{
    Q_ASSERT(qobject_cast<DatabaseWidget*>(sender()));
    Q_ASSERT(databaseFromDatabaseWidget(qobject_cast<DatabaseWidget*>(sender())));

    DatabaseWidget* dbWidget = static_cast<DatabaseWidget*>(sender());
    updateTabName(databaseFromDatabaseWidget(dbWidget));
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

    return Q_NULLPTR;
}

DatabaseManagerStruct DatabaseTabWidget::indexDatabaseManagerStruct(int index)
{
    QWidget* dbWidget = widget(index);

    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().dbWidget == dbWidget) {
            return i.value();
        }
    }

    return DatabaseManagerStruct();
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

    return Q_NULLPTR;
}

void DatabaseTabWidget::insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct, int index)
{
    m_dbList.insert(db, dbStruct);

    index = insertTab(index, dbStruct.dbWidget, "");
    toggleTabbar();
    updateTabName(db);
    setCurrentIndex(index);
    connectDatabase(db);
    connect(dbStruct.dbWidget, SIGNAL(closeRequest()), SLOT(closeDatabaseFromSender()));
    connect(dbStruct.dbWidget, SIGNAL(databaseChanged(Database*)), SLOT(changeDatabase(Database*)));
    connect(dbStruct.dbWidget, SIGNAL(unlockedDatabase()), SLOT(updateTabNameFromDbWidgetSender()));
}

DatabaseWidget* DatabaseTabWidget::currentDatabaseWidget()
{
    Database* db = indexDatabase(currentIndex());
    if (db) {
        return m_dbList[db].dbWidget;
    }
    else {
        return Q_NULLPTR;
    }
}

bool DatabaseTabWidget::hasLockableDatabases()
{
    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        DatabaseWidget::Mode mode = i.value().dbWidget->currentMode();

        if ((mode == DatabaseWidget::ViewMode ||
             mode == DatabaseWidget::EditMode ||
             mode == DatabaseWidget::OpenMode)
                && i.value().dbWidget->dbHasKey()) {
            return true;
        }
    }

    return false;
}

void DatabaseTabWidget::lockDatabases()
{
    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        DatabaseWidget::Mode mode = i.value().dbWidget->currentMode();

        if ((mode == DatabaseWidget::ViewMode ||
             mode == DatabaseWidget::EditMode ||
             mode == DatabaseWidget::OpenMode)
                && i.value().dbWidget->dbHasKey()) {
            i.value().dbWidget->lock();
            updateTabName(i.key());
        }
    }
}

void DatabaseTabWidget::modified()
{
    Q_ASSERT(qobject_cast<Database*>(sender()));

    Database* db = static_cast<Database*>(sender());
    DatabaseManagerStruct& dbStruct = m_dbList[db];

    if (config()->get("AutoSaveAfterEveryChange").toBool() && dbStruct.saveToFilename) {
        saveDatabase(db);
        return;
    }

    if (!dbStruct.modified) {
        dbStruct.modified = true;
        updateTabName(db);
    }
}

void DatabaseTabWidget::updateRecentDatabases(const QString& filename)
{
    if (!config()->get("RememberLastDatabases").toBool()) {
        config()->set("LastDatabases", QVariant());
    }
    else {
        QStringList lastDatabases = config()->get("LastDatabases", QVariant()).toStringList();
        lastDatabases.prepend(filename);
        lastDatabases.removeDuplicates();

        while (lastDatabases.count() > LastDatabasesCount) {
            lastDatabases.removeLast();
        }
        config()->set("LastDatabases", lastDatabases);
    }
}

void DatabaseTabWidget::changeDatabase(Database* newDb)
{
    Q_ASSERT(sender());
    Q_ASSERT(!m_dbList.contains(newDb));

    DatabaseWidget* dbWidget = static_cast<DatabaseWidget*>(sender());
    Database* oldDb = databaseFromDatabaseWidget(dbWidget);
    DatabaseManagerStruct dbStruct = m_dbList[oldDb];
    m_dbList.remove(oldDb);
    m_dbList.insert(newDb, dbStruct);

    updateTabName(newDb);
    connectDatabase(newDb, oldDb);
}

void DatabaseTabWidget::connectDatabase(Database* newDb, Database* oldDb)
{
    if (oldDb) {
        oldDb->disconnect(this);
    }

    connect(newDb, SIGNAL(nameTextChanged()), SLOT(updateTabNameFromDbSender()));
    connect(newDb, SIGNAL(modified()), SLOT(modified()));
    newDb->setEmitModified(true);
}

void DatabaseTabWidget::performGlobalAutoType()
{
    QList<Database*> unlockedDatabases;

    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        DatabaseWidget::Mode mode = i.value().dbWidget->currentMode();

        if (mode != DatabaseWidget::LockedMode) {
            unlockedDatabases.append(i.key());
        }
    }

    autoType()->performGlobalAutoType(unlockedDatabases);
}
