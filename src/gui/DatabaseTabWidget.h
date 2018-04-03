/*
 * Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 * Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include <QFileInfo>
#include <QHash>
#include <QTabWidget>

#include "gui/DatabaseWidget.h"
#include "gui/MessageWidget.h"

class DatabaseWidget;
class DatabaseWidgetStateSync;
class DatabaseOpenWidget;
class QFile;
class MessageWidget;

struct DatabaseManagerStruct
{
    DatabaseManagerStruct();

    DatabaseWidget* dbWidget;
    QFileInfo fileInfo;
    bool modified;
    bool readOnly;
    int saveAttempts;
};

Q_DECLARE_TYPEINFO(DatabaseManagerStruct, Q_MOVABLE_TYPE);

class DatabaseTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit DatabaseTabWidget(QWidget* parent = nullptr);
    ~DatabaseTabWidget() override;
    void openDatabase(const QString& fileName, const QString& pw = QString(), const QString& keyFile = QString());
    void mergeDatabase(const QString& fileName);
    DatabaseWidget* currentDatabaseWidget();
    bool hasLockableDatabases() const;

    static const int LastDatabasesCount;

public slots:
    void newDatabase();
    void openDatabase();
    void importCsv();
    void mergeDatabase();
    void importKeePass1Database();
    bool saveDatabase(int index = -1);
    bool saveDatabaseAs(int index = -1);
    void exportToCsv();
    bool closeDatabase(int index = -1);
    void closeDatabaseFromSender();
    bool closeAllDatabases();
    void changeMasterKey();
    void changeDatabaseSettings();
    bool readOnly(int index = -1);
    bool canSave(int index = -1);
    bool isModified(int index = -1);
    void performGlobalAutoType();
    void lockDatabases();
    void relockPendingDatabase();
    QString databasePath(int index = -1);

signals:
    void tabNameChanged();
    void databaseWithFileClosed(QString filePath);
    void activateDatabaseChanged(DatabaseWidget* dbWidget);
    void databaseLocked(DatabaseWidget* dbWidget);
    void databaseUnlocked(DatabaseWidget* dbWidget);
    void messageGlobal(const QString&, MessageWidget::MessageType type);
    void messageTab(const QString&, MessageWidget::MessageType type);
    void messageDismissGlobal();
    void messageDismissTab();

private slots:
    void updateTabName(Database* db);
    void updateTabNameFromDbSender();
    void updateTabNameFromDbWidgetSender();
    void modified();
    void toggleTabbar();
    void changeDatabase(Database* newDb, bool unsavedChanges);
    void emitActivateDatabaseChanged();
    void emitDatabaseUnlockedFromDbWidgetSender();

private:
    bool saveDatabase(Database* db, QString filePath = "");
    bool saveDatabaseAs(Database* db);
    bool closeDatabase(Database* db);
    void deleteDatabase(Database* db);
    int databaseIndex(Database* db);
    Database* indexDatabase(int index);
    DatabaseManagerStruct indexDatabaseManagerStruct(int index);
    Database* databaseFromDatabaseWidget(DatabaseWidget* dbWidget);
    void insertDatabase(Database* db, const DatabaseManagerStruct& dbStruct);
    void updateLastDatabases(const QString& filename);
    void connectDatabase(Database* newDb, Database* oldDb = nullptr);

    QHash<Database*, DatabaseManagerStruct> m_dbList;
    QPointer<DatabaseWidgetStateSync> m_dbWidgetStateSync;
    QPointer<DatabaseWidget> m_dbPendingLock;
};

#endif // KEEPASSX_DATABASETABWIDGET_H
