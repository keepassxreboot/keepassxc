/*
 * Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_DATABASETABWIDGET_H
#define KEEPASSXC_DATABASETABWIDGET_H

#include "DatabaseOpenDialog.h"
#include "config-keepassx.h"
#include "gui/MessageWidget.h"
#include "wizard/ImportWizard.h"

#include <QTabWidget>
#include <QTimer>

class Database;
class DatabaseWidget;
class DatabaseWidgetStateSync;
class DatabaseOpenWidget;
class RemoteDatabaseManager;

class DatabaseTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit DatabaseTabWidget(QWidget* parent = nullptr);
    ~DatabaseTabWidget() override;
    void mergeDatabase(const QString& filePath);

    QString tabName(int index);
    DatabaseWidget* currentDatabaseWidget();
    DatabaseWidget* databaseWidgetFromIndex(int index) const;

    bool canSave(int index = -1) const;
    bool isModified(int index = -1) const;
    bool hasLockableDatabases() const;

    // Remote database support
    void openRemoteDatabase(const QString& url,
                           const QString& password = {},
                           const QString& keyfile = {});
    bool isRemoteDatabase(const QString& filePath) const;
    void saveRemoteDatabase(int index = -1);

public slots:
    void lockAndSwitchToFirstUnlockedDatabase(int index = -1);
    void addDatabaseTab(const QString& filePath,
                        bool inBackground = false,
                        const QString& password = {},
                        const QString& keyfile = {});
    void addDatabaseTab(DatabaseWidget* dbWidget, bool inBackground = false);
    bool closeDatabaseTab(int index);
    bool closeDatabaseTab(DatabaseWidget* dbWidget);
    bool closeAllDatabaseTabs();
    void lockDatabases();
    void lockAllDatabases();
    void performGlobalAutoType();
    void showDatabaseReports();
    void importCsv();
    void importKeePass1Database();
    void mergeDatabase();
    void exportToCsv();
    void exportToHtml();

signals:
    void activeDatabaseChanged(DatabaseWidget* dbWidget);
    void databaseTabActivated(bool enabled);
    void messageGlobal(const QString& text, MessageWidget::MessageType type);
    void messageDismissGlobal();

private slots:
    void emitActiveDatabaseChanged();
    void updateTabName(int index);
    void toggleTabVisibility();
    void handleDatabaseUnlock();
    void handleDatabaseLock();
    void handleDatabaseModified();
    void handleDatabaseSaved();
    void handleRemoteDatabaseError(const QString& url, const QString& error);

private:
    void addDatabaseTab(DatabaseWidget* dbWidget,
                        bool inBackground,
                        const QString& filePath);
    bool closeDatabaseTab(int index, bool lock);
    int databaseTabIndex(DatabaseWidget* dbWidget);
    bool saveDatabase(int index = -1, bool force = false);
    bool saveDatabaseAs(int index = -1);
    bool saveDatabaseBackup(int index = -1);
    void updateLastUsedDatabase(const QString& filePath);

    DatabaseWidgetStateSync* m_dbWidgetStateSync;
    DatabaseWidget* m_dbWidgetPendingLock;
    DatabaseOpenDialog* m_databaseOpenDialog;
    ImportWizard* m_importWizard;
    bool m_databaseOpenInProgress;
    RemoteDatabaseManager* m_remoteDatabaseManager;
    QHash<QString, QString> m_remoteDatabaseUrls; // Maps local cache path to remote URL
};

#endif // KEEPASSXC_DATABASETABWIDGET_H