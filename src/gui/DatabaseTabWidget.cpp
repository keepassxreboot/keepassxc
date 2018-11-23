/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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
#include "gui/wizard/NewDatabaseWizard.h"

DatabaseTabWidget::DatabaseTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_dbWidgetStateSync(new DatabaseWidgetStateSync(this))
    , m_dbPendingLock(nullptr)
{
    auto* tabBar = new DragTabBar(this);
    setTabBar(tabBar);
    setDocumentMode(true);

    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabaseTab(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(emitActivateDatabaseChanged()));
    connect(this, SIGNAL(activateDatabaseChanged(DatabaseWidget*)), m_dbWidgetStateSync, SLOT(setActive(DatabaseWidget*)));
    connect(autoType(), SIGNAL(globalShortcutTriggered()), SLOT(performGlobalAutoType()));
    connect(autoType(), SIGNAL(autotypePerformed()), SLOT(relockPendingDatabase()));
}

DatabaseTabWidget::~DatabaseTabWidget()
{
}

void DatabaseTabWidget::toggleTabbar()
{
    if (count() > 1) {
        tabBar()->show();
    } else {
        tabBar()->hide();
    }
}

/**
 * Helper method for invoking the new database wizard.
 * The user of this method MUST take ownership of the returned pointer.
 *
 * @return pointer to the configured new database, nullptr on failure
 */
QSharedPointer<Database> DatabaseTabWidget::execNewDatabaseWizard()
{
    // use QScopedPointer to ensure deletion after scope ends, but still parent
    // it to this to make it modal and allow easier access in unit tests
    QScopedPointer<NewDatabaseWizard> wizard(new NewDatabaseWizard(this));
    if (!wizard->exec()) {
        return {};
    }

    auto db = wizard->takeDatabase();
    if (!db) {
        return {};
    }
    Q_ASSERT(db->key());
    Q_ASSERT(db->kdf());
    if (!db->key() || !db->kdf()) {
        MessageBox::critical(this, tr("Database creation error"),
            tr("The created database has no key or KDF, refusing to save it.\n"
               "This is definitely a bug, please report it to the developers."),
            QMessageBox::Ok, QMessageBox::Ok);
        return {};
    }

    return db;
}

void DatabaseTabWidget::newDatabase()
{
    auto db = execNewDatabaseWizard();
    if (!db) {
        return;
    }

    addDatabaseTab(new DatabaseWidget(db, this));
    db->markAsModified();
}

void DatabaseTabWidget::openDatabase()
{
    QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Open database"), "", filter);
    if (!fileName.isEmpty()) {
        addDatabaseTab(fileName);
    }
}

/**
 * Add a new database tab or switch to an existing one if the
 * database has been opened already.
 *
 * @param filePath database file path
 * @param password optional, password to unlock database
 * @param inBackground optional, don't focus tab after opening
 */
void DatabaseTabWidget::addDatabaseTab(const QString& filePath, const QString& password, bool inBackground)
{
    QFileInfo fileInfo(filePath);
    QString canonicalFilePath = fileInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty()) {
        emit messageGlobal(tr("The database file does not exist or is not accessible."), MessageWidget::Error);
        return;
    }

    for (int i = 0, c = count(); i < c; ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        Q_ASSERT(dbWidget);
        if (dbWidget && dbWidget->database()->filePath() == canonicalFilePath) {
            // Does nothing if already unlocked
            dbWidget->performUnlockDatabase(password);
            if (!inBackground) {
                // switch to existing tab if file is already open
                setCurrentIndex(indexOf(dbWidget));
            }
            return;
        }
    }

    auto* dbWidget = new DatabaseWidget(QSharedPointer<Database>::create(filePath), this);
    addDatabaseTab(dbWidget, inBackground);
    dbWidget->performUnlockDatabase(password);
    updateLastDatabases(filePath);
}

/**
 * Add a new database tab containing the given DatabaseWidget
 * @param filePath
 * @param inBackground optional, don't focus tab after opening
 */
void DatabaseTabWidget::addDatabaseTab(DatabaseWidget* dbWidget, bool inBackground)
{
    Q_ASSERT(dbWidget->database());

    int index = addTab(dbWidget, "");
    updateTabName(index);
    toggleTabbar();

    if (!inBackground) {
        setCurrentIndex(index);
    }

    connect(dbWidget, SIGNAL(databaseFilePathChanged(QString,QString)), SLOT(updateTabName()));
    connect(dbWidget, SIGNAL(requestOpenDatabase(QString,QString,bool)), SLOT(addDatabaseTab(QString,QString,bool)));
    connect(dbWidget, SIGNAL(closeRequest()), SLOT(closeDatabaseTabFromSender()));
    connect(dbWidget, SIGNAL(databaseModified()), SLOT(updateTabName()));
    connect(dbWidget, SIGNAL(databaseSaved()), SLOT(updateTabName()));
    connect(dbWidget, SIGNAL(databaseUnlocked()), SLOT(updateTabName()));
    connect(dbWidget, SIGNAL(databaseUnlocked()), SLOT(emitDatabaseLockChanged()));
    connect(dbWidget, SIGNAL(databaseLocked()), SLOT(updateTabName()));
    connect(dbWidget, SIGNAL(databaseLocked()), SLOT(emitDatabaseLockChanged()));
}

void DatabaseTabWidget::importCsv()
{
    QString filter = QString("%1 (*.csv);;%2 (*)").arg(tr("CSV file"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Select CSV file"), "", filter);

    if (fileName.isEmpty()) {
        return;
    }

    auto db = execNewDatabaseWizard();
    if (!db) {
        return;
    }

    auto* dbWidget = new DatabaseWidget(db, this);
    addDatabaseTab(dbWidget);
    dbWidget->switchToCsvImport(fileName);
}

void DatabaseTabWidget::mergeDatabase()
{
    auto dbWidget = currentDatabaseWidget();
    if (dbWidget && !dbWidget->isLocked()) {
        QString filter = QString("%1 (*.kdbx);;%2 (*)").arg(tr("KeePass 2 Database"), tr("All files"));
        const QString fileName = fileDialog()->getOpenFileName(this, tr("Merge database"), "", filter);
        if (!fileName.isEmpty()) {
            mergeDatabase(fileName);
        }
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

    auto db = QSharedPointer<Database>::create();
    auto* dbWidget = new DatabaseWidget(db, this);
    addDatabaseTab(dbWidget);
    dbWidget->switchToImportKeepass1(fileName);
}

/**
 * Attempt to close the current database and remove its tab afterwards.
 *
 * @param index index of the database tab to close
 * @return true if database was closed successully
 */
bool DatabaseTabWidget::closeCurrentDatabaseTab()
{
    return closeDatabaseTab(currentIndex());
}

/**
 * Attempt to close the database tab that sent the close request.
 *
 * @param index index of the database tab to close
 * @return true if database was closed successully
 */
bool DatabaseTabWidget::closeDatabaseTabFromSender()
{
    return closeDatabaseTab(qobject_cast<DatabaseWidget*>(sender()));
}

/**
 * Attempt to close a database and remove its tab afterwards.
 *
 * @param index index of the database tab to close
 * @return true if database was closed successully
 */
bool DatabaseTabWidget::closeDatabaseTab(int index)
{
    return closeDatabaseTab(qobject_cast<DatabaseWidget*>(widget(index)));
}

/**
 * Attempt to close a database and remove its tab afterwards.
 *
 * @param dbWidget \link DatabaseWidget to close
 * @return true if database was closed successully
 */
bool DatabaseTabWidget::closeDatabaseTab(DatabaseWidget* dbWidget)
{
    int tabIndex = indexOf(dbWidget);
    if (!dbWidget || tabIndex < 0) {
        return false;
    }

    QString filePath = dbWidget->database()->filePath();
    if (!dbWidget->close()) {
        return false;
    }

    removeTab(tabIndex);
    dbWidget->deleteLater();
    toggleTabbar();
    emit databaseClosed(filePath);
    return true;
}

/**
 * Attempt to close all opened databases.
 * The attempt will be aborted with the first database that cannot be closed.
 *
 * @return true if all databases could be closed.
 */
bool DatabaseTabWidget::closeAllDatabaseTabs()
{
    while (count() > 0) {
        if (!closeDatabaseTab(0)) {
            return false;
        }
    }

    return true;
}

bool DatabaseTabWidget::saveDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    return databaseWidgetFromIndex(index)->save();
}

bool DatabaseTabWidget::saveDatabaseAs(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    bool ok = dbWidget->saveAs();
    if (ok) {
        updateLastDatabases(dbWidget->database()->filePath());
    }
    return ok;
}

void DatabaseTabWidget::closeDatabaseFromSender()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    Q_ASSERT(dbWidget);
    closeDatabaseTab(dbWidget);
}

void DatabaseTabWidget::exportToCsv()
{
    auto db = databaseWidgetFromIndex(currentIndex())->database();
    if (!db) {
        Q_ASSERT(false);
        return;
    }

    QString fileName = fileDialog()->getSaveFileName(
        this, tr("Export database to CSV file"), QString(), tr("CSV file").append(" (*.csv)"), nullptr, nullptr, "csv");
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

bool DatabaseTabWidget::isReadOnly(int index) const
{
    if (count() == 0) {
        return false;
    }

    if (index == -1) {
        index = currentIndex();
    }

    auto db = databaseWidgetFromIndex(index)->database();
    return db && db->isReadOnly();
}

bool DatabaseTabWidget::isModified(int index) const
{
    if (count() == 0) {
        return false;
    }

    if (index == -1) {
        index = currentIndex();
    }

    auto db = databaseWidgetFromIndex(index)->database();
    return db && db->isModified();
}

bool DatabaseTabWidget::canSave(int index) const
{
    return !isReadOnly(index) && isModified(index);
}

bool DatabaseTabWidget::hasLockableDatabases() const
{
    for (int i = 0, c = count(); i < c; ++i) {
        if (!databaseWidgetFromIndex(i)->isLocked()) {
            return true;
        }
    }
    return false;
}

/**
 * Get the tab's (original) display name without platform-specific
 * mangling that may occur when reading back the actual widget's \link tabText()
 *
 * @param index tab index
 * @return tab name
 */
QString DatabaseTabWidget::tabName(int index)
{
    if (index == -1 || index > count()) {
        return "";
    }

    auto* dbWidget = databaseWidgetFromIndex(index);

    auto db = dbWidget->database();
    Q_ASSERT(db);
    if (!db) {
        return "";
    }

    QString tabName;

    if (!db->filePath().isEmpty()) {
        QFileInfo fileInfo(db->filePath());

        if (db->metadata()->name().isEmpty()) {
            tabName = fileInfo.fileName();
        } else {
            tabName = db->metadata()->name();
        }

        setTabToolTip(index, fileInfo.absoluteFilePath());
    } else {
        if (db->metadata()->name().isEmpty()) {
            tabName = tr("New Database");
        } else {
            tabName = tr("%1 [New Database]", "Database tab name modifier").arg(db->metadata()->name());
        }
    }

    if (dbWidget->isLocked()) {
        tabName = tr("%1 [Locked]", "Database tab name modifier").arg(tabName);
    }

    if (db->isReadOnly()) {
        tabName = tr("%1 [Read-only]", "Database tab name modifier").arg(tabName);
    }

    if (db->isModified()) {
        tabName.append("*");
    }

    return tabName;
}

/**
 * Update of the given tab index or of the sending
 * DatabaseWidget if `index` == -1.
 */
void DatabaseTabWidget::updateTabName(int index)
{
    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget) {
        dbWidget = qobject_cast<DatabaseWidget*>(sender());
    }
    Q_ASSERT(dbWidget);
    if (!dbWidget) {
        return;
    }
    index = indexOf(dbWidget);
    setTabText(index, tabName(index));
    emit tabNameChanged();
}

DatabaseWidget* DatabaseTabWidget::databaseWidgetFromIndex(int index) const
{
    return qobject_cast<DatabaseWidget*>(widget(index));
}

DatabaseWidget* DatabaseTabWidget::currentDatabaseWidget()
{
    return qobject_cast<DatabaseWidget*>(currentWidget());
}

void DatabaseTabWidget::lockDatabases()
{
    for (int i = 0, c = count(); i < c; ++i) {
        if (!databaseWidgetFromIndex(i)->lock()) {
            return;
        }
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

    if (m_dbPendingLock->isLocked() || !m_dbPendingLock->database()->hasKey()) {
        m_dbPendingLock = nullptr;
        return;
    }

    m_dbPendingLock->lock();
    m_dbPendingLock = nullptr;
}

void DatabaseTabWidget::updateLastDatabases(const QString& filename)
{
    if (!config()->get("RememberLastDatabases").toBool()) {
        config()->set("LastDatabases", QVariant());
    } else {
        QStringList lastDatabases = config()->get("LastDatabases", QVariant()).toStringList();
        lastDatabases.prepend(filename);
        lastDatabases.removeDuplicates();

        while (lastDatabases.count() > config()->get("NumberOfRememberedLastDatabases").toInt()) {
            lastDatabases.removeLast();
        }
        config()->set("LastDatabases", lastDatabases);
    }
}

void DatabaseTabWidget::emitActivateDatabaseChanged()
{
    emit activateDatabaseChanged(currentDatabaseWidget());
}

void DatabaseTabWidget::emitDatabaseLockChanged()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    Q_ASSERT(dbWidget);
    if (!dbWidget) {
        return;
    }

    if (dbWidget->isLocked()) {
        emit databaseLocked(dbWidget);
    } else {
        emit databaseUnlocked(dbWidget);
    }
}

void DatabaseTabWidget::performGlobalAutoType()
{
    QList<QSharedPointer<Database>> unlockedDatabases;

    for (int i = 0, c = count(); i < c; ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        if (!dbWidget->isLocked()) {
            unlockedDatabases.append(dbWidget->database());
        }
    }

    if (!unlockedDatabases.isEmpty()) {
        autoType()->performGlobalAutoType(unlockedDatabases);
    } else if (count() > 0) {
        // TODO: allow for database selection during Auto-Type instead of using the first tab
        m_dbPendingLock = databaseWidgetFromIndex(0);
        m_dbPendingLock->prepareUnlock();
    }
}
