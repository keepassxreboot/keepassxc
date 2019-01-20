/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_DATABASETABWIDGET_H
#define KEEPASSX_DATABASETABWIDGET_H

#include "DatabaseOpenDialog.h"
#include "gui/MessageWidget.h"

#include <QPointer>
#include <QTabWidget>

class Database;
class DatabaseWidget;
class DatabaseWidgetStateSync;
class DatabaseOpenWidget;

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

    bool isReadOnly(int index = -1) const;
    bool canSave(int index = -1) const;
    bool isModified(int index = -1) const;
    bool hasLockableDatabases() const;

public slots:
    void addDatabaseTab(const QString& filePath, bool inBackground = false, const QString& password = {});
    void addDatabaseTab(DatabaseWidget* dbWidget, bool inBackground = false);
    bool closeDatabaseTab(int index);
    bool closeDatabaseTab(DatabaseWidget* dbWidget);
    bool closeAllDatabaseTabs();
    bool closeCurrentDatabaseTab();
    bool closeDatabaseTabFromSender();
    void updateTabName(int index = -1);

    void newDatabase();
    void openDatabase();
    void mergeDatabase();
    void importCsv();
    void importKeePass1Database();
    bool saveDatabase(int index = -1);
    bool saveDatabaseAs(int index = -1);
    void exportToCsv();

    void lockDatabases();
    void closeDatabaseFromSender();
    void unlockDatabaseInDialog(DatabaseWidget* dbWidget, DatabaseOpenDialog::Intent intent);
    void unlockDatabaseInDialog(DatabaseWidget* dbWidget, DatabaseOpenDialog::Intent intent, const QString& filePath);
    void relockPendingDatabase();

    void changeMasterKey();
    void changeDatabaseSettings();
    void performGlobalAutoType();

signals:
    void databaseClosed(const QString& filePath);
    void databaseUnlocked(DatabaseWidget* dbWidget);
    void databaseLocked(DatabaseWidget* dbWidget);
    void activateDatabaseChanged(DatabaseWidget* dbWidget);
    void tabNameChanged();
    void messageGlobal(const QString&, MessageWidget::MessageType type);
    void messageDismissGlobal();

private slots:
    void toggleTabbar();
    void emitActivateDatabaseChanged();
    void emitDatabaseLockChanged();

private:
    QSharedPointer<Database> execNewDatabaseWizard();
    void updateLastDatabases(const QString& filename);

    QPointer<DatabaseWidgetStateSync> m_dbWidgetStateSync;
    QPointer<DatabaseWidget> m_dbWidgetPendingLock;
    QScopedPointer<DatabaseOpenDialog> m_databaseOpenDialog;
};

#endif // KEEPASSX_DATABASETABWIDGET_H
