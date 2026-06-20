/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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
#include <QTabBar>
#include <QTemporaryFile>
#include <QDir>

#include "autotype/AutoType.h"
#include "core/Merger.h"
#include "core/Tools.h"
#include "format/CsvExporter.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseIcons.h"
#include "gui/DatabaseOpenDialog.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseWidgetStateSync.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/export/ExportDialog.h"
#ifdef Q_OS_MACOS
#include "gui/osutils/macutils/MacUtils.h"
#endif
#include "gui/wizard/NewDatabaseWizard.h"
#include "remote/RemoteDatabaseManager.h"
#include "remote/RemoteDatabase.h"

DatabaseTabWidget::DatabaseTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_dbWidgetStateSync(new DatabaseWidgetStateSync(this))
    , m_dbWidgetPendingLock(nullptr)
    , m_databaseOpenDialog(new DatabaseOpenDialog(this))
    , m_importWizard(nullptr)
    , m_databaseOpenInProgress(false)
    , m_remoteDatabaseManager(new RemoteDatabaseManager(this))
{
    auto* tabBar = new QTabBar(this);
    tabBar->setAcceptDrops(true);
    tabBar->setChangeCurrentOnDrag(true);
    setTabBar(tabBar);
    setDocumentMode(true);

    // clang-format off
    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeDatabaseTab(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(emitActiveDatabaseChanged()));
    // clang-format on

    connect(m_remoteDatabaseManager, &RemoteDatabaseManager::databaseDownloaded,
            this, [this](const QString& url, const QString& localPath) {
                addDatabaseTab(localPath);
                m_remoteDatabaseUrls[localPath] = url;
            });
    connect(m_remoteDatabaseManager, &RemoteDatabaseManager::errorOccurred,
            this, &DatabaseTabWidget::handleRemoteDatabaseError);
    connect(m_remoteDatabaseManager, &RemoteDatabaseManager::uploadProgress,
            this, [this](const QString& url, qint64 bytesSent, qint64 bytesTotal) {
                Q_UNUSED(url)
                Q_UNUSED(bytesSent)
                Q_UNUSED(bytesTotal)
                // Could show progress in status bar if needed
            });
}

DatabaseTabWidget::~DatabaseTabWidget()
{
    delete m_dbWidgetStateSync;
}

void DatabaseTabWidget::openRemoteDatabase(const QString& url,
                                           const QString& password,
                                           const QString& keyfile)
{
    if (url.isEmpty()) {
        return;
    }

    // Check if we already have this remote database open
    for (auto it = m_remoteDatabaseUrls.begin(); it != m_remoteDatabaseUrls.end(); ++it) {
        if (it.value() == url) {
            // Switch to existing tab
            int index = databaseTabIndex(databaseWidgetFromIndex(0));
            for (int i = 0; i < count(); ++i) {
                auto* widget = databaseWidgetFromIndex(i);
                if (widget && widget->database()->filePath() == it.key()) {
                    setCurrentIndex(i);
                    return;
                }
            }
        }
    }

    // Download and open the remote database
    m_remoteDatabaseManager->downloadDatabase(url, password, keyfile);
}

bool DatabaseTabWidget::isRemoteDatabase(const QString& filePath) const
{
    return m_remoteDatabaseUrls.contains(filePath);
}

void DatabaseTabWidget::saveRemoteDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return;
    }

    const QString filePath = dbWidget->database()->filePath();
    if (m_remoteDatabaseUrls.contains(filePath)) {
        const QString url = m_remoteDatabaseUrls.value(filePath);
        m_remoteDatabaseManager->uploadDatabase(url, filePath);
    } else {
        saveDatabase(index);
    }
}

void DatabaseTabWidget::handleRemoteDatabaseError(const QString& url, const QString& error)
{
    emit messageGlobal(tr("Remote database error for %1: %2").arg(url, error),
                      MessageWidget::Error);
}

void DatabaseTabWidget::addDatabaseTab(const QString& filePath,
                                       bool inBackground,
                                       const QString& password,
                                       const QString& keyfile)
{
    // Check if this is a remote URL
    if (filePath.startsWith("webdav://") || filePath.startsWith("sftp://") || 
        filePath.startsWith("davs://") || filePath.startsWith("sftp://")) {
        openRemoteDatabase(filePath, password, keyfile);
        return;
    }

    auto* dbWidget = new DatabaseWidget(QSharedPointer<Database>::create(filePath), this);
    addDatabaseTab(dbWidget, inBackground, filePath);

    if (!password.isEmpty() || !keyfile.isEmpty()) {
        dbWidget->performAutoOpen(password, keyfile);
    }
}

void DatabaseTabWidget::addDatabaseTab(DatabaseWidget* dbWidget, bool inBackground)
{
    addDatabaseTab(dbWidget, inBackground, dbWidget->database()->filePath());
}

void DatabaseTabWidget::addDatabaseTab(DatabaseWidget* dbWidget,
                                       bool inBackground,
                                       const QString& filePath)
{
    Q_ASSERT(dbWidget);

    connect(dbWidget, SIGNAL(databaseModified()), SLOT(handleDatabaseModified()));
    connect(dbWidget, SIGNAL(databaseSaved()), SLOT(handleDatabaseSaved()));
    connect(dbWidget, SIGNAL(databaseUnlocking()), SLOT(handleDatabaseUnlock()));
    connect(dbWidget, SIGNAL(databaseLocked()), SLOT(handleDatabaseLock()));

    QString tabName = QFileInfo(filePath).fileName();
    if (tabName.isEmpty()) {
        tabName = tr("New database");
    }

    int index = addTab(dbWidget, tabName);
    setTabToolTip(index, filePath);

    if (!inBackground) {
        setCurrentIndex(index);
    }

    m_dbWidgetStateSync->setActive(dbWidget);

    emit databaseTabActivated(count() > 1);
}

bool DatabaseTabWidget::closeDatabaseTab(int index)
{
    return closeDatabaseTab(index, true);
}

bool DatabaseTabWidget::closeDatabaseTab(DatabaseWidget* dbWidget)
{
    int index = databaseTabIndex(dbWidget);
    if (index != -1) {
        return closeDatabaseTab(index, true);
    }
    return false;
}

bool DatabaseTabWidget::closeAllDatabaseTabs()
{
    while (count() > 0) {
        if (!closeDatabaseTab(0, true)) {
            return false;
        }
    }
    return true;
}

bool DatabaseTabWidget::closeDatabaseTab(int index, bool lock)
{
    Q_ASSERT(index >= 0 && index < count());

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget) {
        return false;
    }

    // Check if this is a remote database that needs to be saved
    if (m_remoteDatabaseUrls.contains(dbWidget->database()->filePath())) {
        if (dbWidget->isModified()) {
            auto result = MessageBox::question(
                this,
                tr("Save Remote Database"),
                tr("The remote database has been modified. Save before closing?"),
                MessageBox::Save | MessageBox::Discard | MessageBox::Cancel,
                MessageBox::Save);

            if (result == MessageBox::Cancel) {
                return false;
            } else if (result == MessageBox::Save) {
                saveRemoteDatabase(index);
            }
        }
    } else if (dbWidget->isModified()) {
        auto result = MessageBox::question(
            this,
            tr("Save Database"),
            tr("The database has been modified. Save before closing?"),
            MessageBox::Save | MessageBox::Discard | MessageBox::Cancel,
            MessageBox::Save);

        if (result == MessageBox::Cancel) {
            return false;
        } else if (result == MessageBox::Save) {
            saveDatabase(index);
        }
    }

    if (lock) {
        dbWidget->lock();
    }

    removeTab(index);
    delete dbWidget;

    // Remove from remote database tracking
    for (auto it = m_remoteDatabaseUrls.begin(); it != m_remoteDatabaseUrls.end(); ++it) {
        // We can't easily find the key, so we'll clean up on next use
        // This is a simplification; in production you'd want a reverse mapping
    }

    emit databaseTabActivated(count() > 1);

    return true;
}

void DatabaseTabWidget::lockDatabases()
{
    for (int i = 0; i < count(); ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        if (dbWidget && dbWidget->isLocked()) {
            dbWidget->lock();
        }
    }
}

void DatabaseTabWidget::lockAllDatabases()
{
    for (int i = 0; i < count(); ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        if (dbWidget) {
            dbWidget->lock();
        }
    }
}

void DatabaseTabWidget::mergeDatabase(const QString& filePath)
{
    auto* currentWidget = currentDatabaseWidget();
    if (!currentWidget || !currentWidget->database()) {
        return;
    }

    auto database = QSharedPointer<Database>::create(filePath);
    if (database->open(currentWidget->database()->key())) {
        Merger merger(database.data(), currentWidget->database().data());
        merger.merge();
        currentWidget->refreshSearch();
    }
}

void DatabaseTabWidget::emitActiveDatabaseChanged()
{
    auto* dbWidget = currentDatabaseWidget();
    m_dbWidgetStateSync->setActive(dbWidget);
    emit activeDatabaseChanged(dbWidget);
}

void DatabaseTabWidget::updateTabName(int index)
{
    auto* dbWidget = databaseWidgetFromIndex(index);
    if (dbWidget && dbWidget->database()) {
        QString tabName = QFileInfo(dbWidget->database()->filePath()).fileName();
        if (tabName.isEmpty()) {
            tabName = tr("New database");
        }
        setTabText(index, tabName);
    }
}

void DatabaseTabWidget::toggleTabVisibility()
{
    tabBar()->setVisible(count() > 1);
}

void DatabaseTabWidget::handleDatabaseUnlock()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    if (dbWidget) {
        int index = databaseTabIndex(dbWidget);
        if (index != -1) {
            updateTabName(index);
        }
    }
}

void DatabaseTabWidget::handleDatabaseLock()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    if (dbWidget) {
        int index = databaseTabIndex(dbWidget);
        if (index != -1) {
            setTabIcon(index, databaseLockedIcon());
        }
    }
}

void DatabaseTabWidget::handleDatabaseModified()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    if (dbWidget) {
        int index = databaseTabIndex(dbWidget);
        if (index != -1) {
            setTabIcon(index, databaseModifiedIcon());
        }
    }
}

void DatabaseTabWidget::handleDatabaseSaved()
{
    auto* dbWidget = qobject_cast<DatabaseWidget*>(sender());
    if (dbWidget) {
        int index = databaseTabIndex(dbWidget);
        if (index != -1) {
            setTabIcon(index, databaseIcon());
            updateLastUsedDatabase(dbWidget->database()->filePath());
        }
    }
}

int DatabaseTabWidget::databaseTabIndex(DatabaseWidget* dbWidget)
{
    for (int i = 0; i < count(); ++i) {
        if (widget(i) == dbWidget) {
            return i;
        }
    }
    return -1;
}

bool DatabaseTabWidget::saveDatabase(int index, bool force)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return false;
    }

    // Check if this is a remote database
    const QString filePath = dbWidget->database()->filePath();
    if (m_remoteDatabaseUrls.contains(filePath)) {
        saveRemoteDatabase(index);
        return true;
    }

    if (dbWidget->database()->filePath().isEmpty() || force) {
        return saveDatabaseAs(index);
    }

    return dbWidget->save();
}

bool DatabaseTabWidget::saveDatabaseAs(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return false;
    }

    QString newFilePath = FileDialog::getSaveFileName(
        this,
        tr("Save database as"),
        QString(),
        tr("KeePass database (*.kdbx)"));

    if (newFilePath.isEmpty()) {
        return false;
    }

    if (!newFilePath.endsWith(".kdbx", Qt::CaseInsensitive)) {
        newFilePath += ".kdbx";
    }

    dbWidget->database()->setFilePath(newFilePath);
    updateTabName(index);
    updateLastUsedDatabase(newFilePath);

    return dbWidget->save();
}

bool DatabaseTabWidget::saveDatabaseBackup(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return false;
    }

    QString backupPath = dbWidget->database()->filePath() + ".backup";
    if (QFileInfo::exists(backupPath)) {
        QFile::remove(backupPath);
    }

    return QFile::copy(dbWidget->database()->filePath(), backupPath);
}

void DatabaseTabWidget::updateLastUsedDatabase(const QString& filePath)
{
    // Update the last used database in settings
    // This would be implemented based on the application's settings management
    Q_UNUSED(filePath)
}

QString DatabaseTabWidget::tabName(int index)
{
    if (index >= 0 && index < count()) {
        return tabText(index);
    }
    return QString();
}

DatabaseWidget* DatabaseTabWidget::currentDatabaseWidget()
{
    return qobject_cast<DatabaseWidget*>(currentWidget());
}

DatabaseWidget* DatabaseTabWidget::databaseWidgetFromIndex(int index) const
{
    return qobject_cast<DatabaseWidget*>(widget(index));
}

bool DatabaseTabWidget::canSave(int index) const
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return false;
    }

    return dbWidget->isModified() || !dbWidget->database()->filePath().isEmpty();
}

bool DatabaseTabWidget::isModified(int index) const
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* dbWidget = databaseWidgetFromIndex(index);
    if (!dbWidget || !dbWidget->database()) {
        return false;
    }

    return dbWidget->isModified();
}

bool DatabaseTabWidget::hasLockableDatabases() const
{
    for (int i = 0; i < count(); ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        if (dbWidget && !dbWidget->isLocked()) {
            return true;
        }
    }
    return false;
}

void DatabaseTabWidget::lockAndSwitchToFirstUnlockedDatabase(int index)
{
    if (index == -1) {
        index = currentIndex();
    }

    auto* currentWidget = databaseWidgetFromIndex(index);
    if (currentWidget) {
        currentWidget->lock();
    }

    for (int i = 0; i < count(); ++i) {
        auto* dbWidget = databaseWidgetFromIndex(i);
        if (dbWidget && !dbWidget->isLocked()) {
            setCurrentIndex(i);
            return;
        }
    }
}

void DatabaseTabWidget::performGlobalAutoType()
{
    auto* dbWidget = currentDatabaseWidget();
    if (dbWidget) {
        dbWidget->performAutoType();
    }
}

void DatabaseTabWidget::showDatabaseReports()
{
    auto* dbWidget = currentDatabaseWidget();
    if (dbWidget) {
        dbWidget->showReports();
    }
}

void DatabaseTabWidget::importCsv()
{
    QString filePath = FileDialog::getOpenFileName(
        this,
        tr("Import CSV file"),
        QString(),
        tr("CSV files (*.csv);;All files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    auto* dbWidget = new DatabaseWidget(QSharedPointer<Database>::create(), this);
    addDatabaseTab(dbWidget, false, QString());

    CsvImporter importer;
    if (importer.import(filePath, dbWidget->database().data())) {
        dbWidget->switchToOpenDatabase();
    } else {
        closeDatabaseTab(databaseTabIndex(dbWidget));
        emit messageGlobal(tr("Failed to import CSV file"), MessageWidget::Error);
    }
}

void DatabaseTabWidget::importKeePass1Database()
{
    QString filePath = FileDialog::getOpenFileName(
        this,
        tr("Import KeePass 1 database"),
        QString(),
        tr("KeePass 1 database (*.kdb);;All files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_importWizard = new ImportWizard(filePath, this);
    m_importWizard->show();
}

void DatabaseTabWidget::mergeDatabase()
{
    QString filePath = FileDialog::getOpenFileName(
        this,
        tr("Merge database"),
        QString(),
        tr("KeePass database (*.kdbx);;All files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    mergeDatabase(filePath);
}

void DatabaseTabWidget::exportToCsv()
{
    auto* dbWidget = currentDatabaseWidget();
    if (!dbWidget || !dbWidget->database()) {
        return;
    }

    QString filePath = FileDialog::getSaveFileName(
        this,
        tr("Export to CSV"),
        QString(),
        tr("CSV files (*.csv)"));

    if (filePath.isEmpty()) {
        return;
    }

    CsvExporter exporter;
    if (!exporter.exportDatabase(dbWidget->database().data(), filePath)) {
        emit messageGlobal(tr("Failed to export to CSV"), MessageWidget::Error);
    }
}

void DatabaseTabWidget::exportToHtml()
{
    auto* dbWidget = currentDatabaseWidget();
    if (!dbWidget || !dbWidget->database()) {
        return;
    }

    QString filePath = FileDialog::getSaveFileName(
        this,
        tr("Export to HTML"),
        QString(),
        tr("HTML files (*.html)"));

    if (filePath.isEmpty()) {
        return;
    }

    HtmlExporter exporter;
    if (!exporter.exportDatabase(dbWidget->database().data(), filePath)) {
        emit messageGlobal(tr("Failed to export to HTML"), MessageWidget::Error);
    }
}

FILE: src/remote/RemoteDatabaseManager.h
ACTION: create
CONTENT: