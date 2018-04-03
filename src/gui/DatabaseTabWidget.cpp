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

#include "DatabaseTabWidget.h"

#include <QFileInfo>
#include <QPushButton>
#include <QTabWidget>

#include "autotype/AutoType.h"
#include "core/AsyncTask.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "format/CsvExporter.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseWidgetStateSync.h"
#include "gui/DragTabBar.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/UnlockDatabaseDialog.h"
#include "gui/entry/EntryView.h"
#include "gui/group/GroupView.h"

DatabaseManagerStruct::DatabaseManagerStruct()
    : dbWidget(nullptr)
    , modified(false)
    , readOnly(false)
    , saveAttempts(0)
{
}

const int DatabaseTabWidget::LastDatabasesCount = 5;

DatabaseTabWidget::DatabaseTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_dbWidgetStateSync(new DatabaseWidgetStateSync(this))
    , m_dbPendingLock(nullptr)
{
    DragTabBar* tabBar = new DragTabBar(this);
    setTabBar(tabBar);
    setDocumentMode(true);

    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabase(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(emitActivateDatabaseChanged()));
    connect(
        this, SIGNAL(activateDatabaseChanged(DatabaseWidget*)), m_dbWidgetStateSync, SLOT(setActive(DatabaseWidget*)));
    connect(autoType(), SIGNAL(globalShortcutTriggered()), SLOT(performGlobalAutoType()));
    connect(autoType(), SIGNAL(autotypePerformed()), SLOT(relockPendingDatabase()));
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
    if (count() > 1) {
        tabBar()->show();
    } else {
        tabBar()->hide();
    }
}

void DatabaseTabWidget::newDatabase()
{
    DatabaseManagerStruct dbStruct;
    Database* db = new Database();
    db->rootGroup()->setName(tr("Root", "Root group"));
    dbStruct.dbWidget = new DatabaseWidget(db, this);

    CompositeKey emptyKey;
    db->setKey(emptyKey);

    insertDatabase(db, dbStruct);

    if (!saveDatabaseAs(db)) {
        closeDatabase(db);
        return;
    }

    dbStruct.dbWidget->switchToMasterKeyChange(true);
}

void DatabaseTabWidget::openDatabase()
{
    QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open database"), QDir::homePath(), filter);
    if (!fileName.isEmpty()) {
        openDatabase(fileName);
    }
}

void DatabaseTabWidget::openDatabase(const QString& fileName, const QString& pw, const QString& keyFile)
{
    QFileInfo fileInfo(fileName);
    QString canonicalFilePath = fileInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty()) {
        emit messageGlobal(tr("File not found!"), MessageWidget::Error);
        return;
    }

    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        if (i.value().fileInfo.canonicalFilePath() == canonicalFilePath) {
            if (!i.value().dbWidget->dbHasKey() && !(pw.isNull() && keyFile.isEmpty())) {
                // If the database is locked and a pw or keyfile is provided, unlock it
                i.value().dbWidget->switchToOpenDatabase(i.value().fileInfo.absoluteFilePath(), pw, keyFile);
            } else {
                setCurrentIndex(databaseIndex(i.key()));
            }
            return;
        }
    }

    DatabaseManagerStruct dbStruct;

    // test if we can read/write or read the file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite)) {
        if (!file.open(QIODevice::ReadOnly)) {
            // can't open
            emit messageGlobal(tr("Unable to open the database.").append("\n").append(file.errorString()),
                               MessageWidget::Error);
            return;
        } else {
            // can only open read-only
            dbStruct.readOnly = true;
        }
    }
    file.close();

    Database* db = new Database();
    dbStruct.dbWidget = new DatabaseWidget(db, this);
    dbStruct.fileInfo = fileInfo;

    insertDatabase(db, dbStruct);

    if (dbStruct.readOnly) {
        emit messageTab(tr("File opened in read only mode."), MessageWidget::Warning);
    }

    updateLastDatabases(dbStruct.fileInfo.absoluteFilePath());

    if (!pw.isNull() || !keyFile.isEmpty()) {
        dbStruct.dbWidget->switchToOpenDatabase(dbStruct.fileInfo.absoluteFilePath(), pw, keyFile);
    } else {
        dbStruct.dbWidget->switchToOpenDatabase(dbStruct.fileInfo.absoluteFilePath());
    }

    emit messageDismissTab();
}

void DatabaseTabWidget::importCsv()
{
    QString filter = QString("%1 (*.csv);;%2 (*)").arg(tr("CSV file"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open CSV file"), QString(), filter);

    if (fileName.isEmpty()) {
        return;
    }

    Database* db = new Database();
    DatabaseManagerStruct dbStruct;
    dbStruct.dbWidget = new DatabaseWidget(db, this);

    insertDatabase(db, dbStruct);
    dbStruct.dbWidget->switchToImportCsv(fileName);
}

void DatabaseTabWidget::mergeDatabase()
{
    QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
    const QString fileName = fileDialog()->getOpenFileName(this, tr("Merge database"), QString(), filter);
    if (!fileName.isEmpty()) {
        mergeDatabase(fileName);
    }
}

void DatabaseTabWidget::mergeDatabase(const QString& fileName)
{
    currentDatabaseWidget()->switchToOpenMergeDatabase(fileName);
}

void DatabaseTabWidget::importKeePass1Database()
{
    QString filter = QString("%1 (*.kdb);;%2 (*)").arg(tr("KeePass 1 database"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open KeePass 1 database"), QString(), filter);

    if (fileName.isEmpty()) {
        return;
    }

    Database* db = new Database();
    DatabaseManagerStruct dbStruct;
    dbStruct.dbWidget = new DatabaseWidget(db, this);
    dbStruct.dbWidget->databaseModified();
    dbStruct.modified = true;

    insertDatabase(db, dbStruct);

    dbStruct.dbWidget->switchToImportKeepass1(fileName);
}

bool DatabaseTabWidget::closeDatabase(Database* db)
{
    Q_ASSERT(db);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);
    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    dbStruct.dbWidget->closeUnlockDialog();
    QString dbName = tabText(index);
    if (dbName.right(1) == "*") {
        dbName.chop(1);
    }
    if (dbStruct.dbWidget->isInEditMode() && db->hasKey() && dbStruct.dbWidget->isEditWidgetModified()) {
        QMessageBox::StandardButton result = MessageBox::question(
            this,
            tr("Close?"),
            tr("\"%1\" is in edit mode.\nDiscard changes and close anyway?").arg(dbName.toHtmlEscaped()),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) {
            return false;
        }
    }
    if (dbStruct.modified) {
        if (config()->get("AutoSaveOnExit").toBool()) {
            if (!saveDatabase(db)) {
                return false;
            }
        } else if (dbStruct.dbWidget->currentMode() != DatabaseWidget::LockedMode) {
            QMessageBox::StandardButton result =
                MessageBox::question(this,
                                     tr("Save changes?"),
                                     tr("\"%1\" was modified.\nSave changes?").arg(dbName.toHtmlEscaped()),
                                     QMessageBox::Yes | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Yes);
            if (result == QMessageBox::Yes) {
                if (!saveDatabase(db)) {
                    return false;
                }
            } else if (result == QMessageBox::Cancel) {
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
    bool emitDatabaseWithFileClosed = dbStruct.fileInfo.exists() && !dbStruct.readOnly;
    QString filePath = dbStruct.fileInfo.absoluteFilePath();

    int index = databaseIndex(db);

    removeTab(index);
    toggleTabbar();
    m_dbList.remove(db);
    delete dbStruct.dbWidget;
    delete db;

    if (emitDatabaseWithFileClosed) {
        emit databaseWithFileClosed(filePath);
    }
}

bool DatabaseTabWidget::closeAllDatabases()
{
    while (!m_dbList.isEmpty()) {
        if (!closeDatabase()) {
            return false;
        }
    }
    return true;
}

bool DatabaseTabWidget::saveDatabase(Database* db, QString filePath)
{
    DatabaseManagerStruct& dbStruct = m_dbList[db];

    // Never allow saving a locked database; it causes corruption
    Q_ASSERT(dbStruct.dbWidget->currentMode() != DatabaseWidget::LockedMode);
    // Release build interlock
    if (dbStruct.dbWidget->currentMode() == DatabaseWidget::LockedMode) {
        // We return true since a save is not required
        return true;
    }

    if (!dbStruct.readOnly) {
        if (filePath.isEmpty()) {
            filePath = dbStruct.fileInfo.canonicalFilePath();
        }

        dbStruct.dbWidget->blockAutoReload(true);
        // TODO: Make this async, but lock out the database widget to prevent re-entrance
        bool useAtomicSaves = config()->get("UseAtomicSaves", true).toBool();
        QString errorMessage = db->saveToFile(filePath, useAtomicSaves, config()->get("BackupBeforeSave").toBool());
        dbStruct.dbWidget->blockAutoReload(false);

        if (errorMessage.isEmpty()) {
            // successfully saved database file
            dbStruct.modified = false;
            dbStruct.saveAttempts = 0;
            dbStruct.fileInfo = QFileInfo(filePath);
            dbStruct.dbWidget->databaseSaved();
            updateTabName(db);
            emit messageDismissTab();
            return true;
        } else {
            dbStruct.modified = true;
            updateTabName(db);

            if (++dbStruct.saveAttempts > 2 && useAtomicSaves) {
                // Saving failed 3 times, issue a warning and attempt to resolve
                auto choice = MessageBox::question(this,
                                                   tr("Disable safe saves?"),
                                                   tr("KeePassXC has failed to save the database multiple times. "
                                                      "This is likely caused by file sync services holding a lock on "
                                                      "the save file.\nDisable safe saves and try again?"),
                                                   QMessageBox::Yes | QMessageBox::No,
                                                   QMessageBox::Yes);
                if (choice == QMessageBox::Yes) {
                    config()->set("UseAtomicSaves", false);
                    return saveDatabase(db, filePath);
                }
                // Reset save attempts without changing anything
                dbStruct.saveAttempts = 0;
            }

            emit messageTab(tr("Writing the database failed.").append("\n").append(errorMessage), MessageWidget::Error);
            return false;
        }
    } else {
        return saveDatabaseAs(db);
    }
}

bool DatabaseTabWidget::saveDatabaseAs(Database* db)
{
    while (true) {
        DatabaseManagerStruct& dbStruct = m_dbList[db];
        QString oldFilePath;
        if (dbStruct.fileInfo.exists()) {
            oldFilePath = dbStruct.fileInfo.absoluteFilePath();
        } else {
            oldFilePath = QDir::toNativeSeparators(QDir::homePath() + "/" + tr("Passwords").append(".kdbx"));
        }
        QString newFilePath = fileDialog()->getSaveFileName(this,
                                                            tr("Save database as"),
                                                            oldFilePath,
                                                            tr("KeePass 2 Database").append(" (*.kdbx)"),
                                                            nullptr,
                                                            0,
                                                            "kdbx");
        if (!newFilePath.isEmpty()) {
            // Ensure we don't recurse back into this function
            dbStruct.readOnly = false;

            if (!saveDatabase(db, newFilePath)) {
                // Failed to save, try again
                continue;
            }

            dbStruct.dbWidget->updateFilePath(dbStruct.fileInfo.absoluteFilePath());
            updateLastDatabases(dbStruct.fileInfo.absoluteFilePath());
            return true;
        }

        // Canceled file selection
        return false;
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

bool DatabaseTabWidget::saveDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return saveDatabase(indexDatabase(index));
}

bool DatabaseTabWidget::saveDatabaseAs(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return saveDatabaseAs(indexDatabase(index));
}

void DatabaseTabWidget::exportToCsv()
{
    Database* db = indexDatabase(currentIndex());
    if (!db) {
        Q_ASSERT(false);
        return;
    }

    QString fileName = fileDialog()->getSaveFileName(
        this, tr("Export database to CSV file"), QString(), tr("CSV file").append(" (*.csv)"), nullptr, 0, "csv");
    if (fileName.isEmpty()) {
        return;
    }

    CsvExporter csvExporter;
    if (!csvExporter.exportDatabase(fileName, db)) {
        emit messageGlobal(tr("Writing the CSV file failed.").append("\n").append(csvExporter.errorString()),
                           MessageWidget::Error);
    }
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

bool DatabaseTabWidget::canSave(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    const DatabaseManagerStruct& dbStruct = indexDatabaseManagerStruct(index);
    return dbStruct.modified && !dbStruct.readOnly;
}

bool DatabaseTabWidget::isModified(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return indexDatabaseManagerStruct(index).modified;
}

QString DatabaseTabWidget::databasePath(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return indexDatabaseManagerStruct(index).fileInfo.absoluteFilePath();
}

void DatabaseTabWidget::updateTabName(Database* db)
{
    int index = databaseIndex(db);
    Q_ASSERT(index != -1);

    const DatabaseManagerStruct& dbStruct = m_dbList.value(db);

    QString tabName;

    if (dbStruct.fileInfo.exists()) {
        if (db->metadata()->name().isEmpty()) {
            tabName = dbStruct.fileInfo.fileName();
        } else {
            tabName = db->metadata()->name();
        }

        setTabToolTip(index, dbStruct.fileInfo.absoluteFilePath());
    } else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New database");
        } else {
            tabName = tr("%1 [New database]", "tab modifier").arg(db->metadata()->name());
        }
    }

    if (dbStruct.dbWidget->currentMode() == DatabaseWidget::LockedMode) {
        tabName = tr("%1 [locked]", "tab modifier").arg(tabName);
    }

    if (dbStruct.modified) {
        tabName.append("*");
    }

    setTabText(index, tabName);
    emit tabNameChanged();
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

    Database* db = dbWidget->database();
    Group* autoload = db->rootGroup()->findChildByName("AutoOpen");
    if (autoload) {
        const DatabaseManagerStruct& dbStruct = m_dbList.value(db);
        QDir dbFolder(dbStruct.fileInfo.canonicalPath());
        for (auto entry : autoload->entries()) {
            if (entry->url().isEmpty() || entry->password().isEmpty()) {
                continue;
            }
            QFileInfo filepath;
            if (entry->url().startsWith("file://")) {
                QUrl url(entry->url());
                filepath.setFile(url.toLocalFile());
            } else {
                filepath.setFile(entry->url());
                if (filepath.isRelative()) {
                    filepath.setFile(dbFolder, entry->url());
                }
            }

            if (!filepath.isFile()) {
                continue;
            }

            openDatabase(filepath.canonicalFilePath(), entry->password(), "");
        }
    }
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

    return nullptr;
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

    return nullptr;
}

void DatabaseTabWidget::insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct)
{
    m_dbList.insert(db, dbStruct);

    addTab(dbStruct.dbWidget, "");
    toggleTabbar();
    updateTabName(db);
    int index = databaseIndex(db);
    setCurrentIndex(index);
    connectDatabase(db);
    connect(dbStruct.dbWidget, SIGNAL(closeRequest()), SLOT(closeDatabaseFromSender()));
    connect(dbStruct.dbWidget, SIGNAL(databaseChanged(Database*, bool)), SLOT(changeDatabase(Database*, bool)));
    connect(dbStruct.dbWidget, SIGNAL(unlockedDatabase()), SLOT(updateTabNameFromDbWidgetSender()));
    connect(dbStruct.dbWidget, SIGNAL(unlockedDatabase()), SLOT(emitDatabaseUnlockedFromDbWidgetSender()));
}

DatabaseWidget* DatabaseTabWidget::currentDatabaseWidget()
{
    Database* db = indexDatabase(currentIndex());
    if (db) {
        return m_dbList[db].dbWidget;
    } else {
        return nullptr;
    }
}

bool DatabaseTabWidget::hasLockableDatabases() const
{
    QHashIterator<Database*, DatabaseManagerStruct> i(m_dbList);
    while (i.hasNext()) {
        i.next();
        DatabaseWidget::Mode mode = i.value().dbWidget->currentMode();

        if ((mode == DatabaseWidget::ViewMode || mode == DatabaseWidget::EditMode) && i.value().dbWidget->dbHasKey()) {
            return true;
        }
    }

    return false;
}

void DatabaseTabWidget::lockDatabases()
{
    clipboard()->clearCopiedText();

    for (int i = 0; i < count(); i++) {
        DatabaseWidget* dbWidget = static_cast<DatabaseWidget*>(widget(i));
        Database* db = databaseFromDatabaseWidget(dbWidget);

        if (dbWidget->currentMode() == DatabaseWidget::LockedMode || !dbWidget->dbHasKey()) {
            continue;
        }

        // show the correct tab widget before we are asking questions about it
        setCurrentWidget(dbWidget);

        if (dbWidget->currentMode() == DatabaseWidget::EditMode && dbWidget->isEditWidgetModified()) {
            QMessageBox::StandardButton result =
                MessageBox::question(this,
                                     tr("Lock database"),
                                     tr("Can't lock the database as you are currently editing it.\nPlease press cancel "
                                        "to finish your changes or discard them."),
                                     QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel);
            if (result == QMessageBox::Cancel) {
                continue;
            }
        }

        if (m_dbList[db].modified) {
            QMessageBox::StandardButton result =
                MessageBox::question(this,
                                     tr("Lock database"),
                                     tr("This database has been modified.\nDo you want to save the database before "
                                        "locking it?\nOtherwise your changes are lost."),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel);
            if (result == QMessageBox::Save) {
                if (!saveDatabase(db)) {
                    continue;
                }
            } else if (result == QMessageBox::Discard) {
                m_dbList[db].modified = false;
                m_dbList[db].dbWidget->databaseSaved();
            } else if (result == QMessageBox::Cancel) {
                continue;
            }
        }

        dbWidget->lock();
        // database has changed so we can't use the db variable anymore
        updateTabName(dbWidget->database());

        emit databaseLocked(dbWidget);
    }
}

/**
 * This function relock the pending database when autotype has been performed successfully
 * A database is marked as pending when it's unlocked after a global Auto-Type invocation
 */
void DatabaseTabWidget::relockPendingDatabase()
{
    if (!m_dbPendingLock || !config()->get("security/relockautotype").toBool()) {
        return;
    }

    if (m_dbPendingLock->currentMode() == DatabaseWidget::LockedMode || !m_dbPendingLock->dbHasKey()) {
        m_dbPendingLock = nullptr;
        return;
    }

    m_dbPendingLock->lock();

    emit databaseLocked(m_dbPendingLock);
    m_dbPendingLock = nullptr;
}

void DatabaseTabWidget::modified()
{
    Q_ASSERT(qobject_cast<Database*>(sender()));

    Database* db = static_cast<Database*>(sender());
    DatabaseManagerStruct& dbStruct = m_dbList[db];

    if (config()->get("AutoSaveAfterEveryChange").toBool() && !dbStruct.readOnly) {
        saveDatabase(db);
        return;
    }

    if (!dbStruct.modified) {
        dbStruct.modified = true;
        dbStruct.dbWidget->databaseModified();
        updateTabName(db);
    }
}

void DatabaseTabWidget::updateLastDatabases(const QString& filename)
{
    if (!config()->get("RememberLastDatabases").toBool()) {
        config()->set("LastDatabases", QVariant());
    } else {
        QStringList lastDatabases = config()->get("LastDatabases", QVariant()).toStringList();
        lastDatabases.prepend(filename);
        lastDatabases.removeDuplicates();

        while (lastDatabases.count() > LastDatabasesCount) {
            lastDatabases.removeLast();
        }
        config()->set("LastDatabases", lastDatabases);
    }
}

void DatabaseTabWidget::changeDatabase(Database* newDb, bool unsavedChanges)
{
    Q_ASSERT(sender());
    Q_ASSERT(!m_dbList.contains(newDb));

    DatabaseWidget* dbWidget = static_cast<DatabaseWidget*>(sender());
    Database* oldDb = databaseFromDatabaseWidget(dbWidget);
    DatabaseManagerStruct dbStruct = m_dbList[oldDb];
    dbStruct.modified = unsavedChanges;
    m_dbList.remove(oldDb);
    m_dbList.insert(newDb, dbStruct);

    updateTabName(newDb);
    connectDatabase(newDb, oldDb);
}

void DatabaseTabWidget::emitActivateDatabaseChanged()
{
    emit activateDatabaseChanged(currentDatabaseWidget());
}

void DatabaseTabWidget::emitDatabaseUnlockedFromDbWidgetSender()
{
    emit databaseUnlocked(static_cast<DatabaseWidget*>(sender()));
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

    if (unlockedDatabases.size() > 0) {
        autoType()->performGlobalAutoType(unlockedDatabases);
    } else if (m_dbList.size() > 0) {
        m_dbPendingLock = indexDatabaseManagerStruct(0).dbWidget;
        m_dbPendingLock->showUnlockDialog();
    }
}
