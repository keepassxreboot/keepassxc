/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_FDOSECRETSPLUGINGUI_H
#define KEEPASSXC_FDOSECRETSPLUGINGUI_H

#include "fdosecrets/FdoSecretsPlugin.h"

#include <QSet>

class DatabaseTabWidget;
class DatabaseWidget;
class Database;
class QWindow;

class FdoSecretsPluginGUI : public FdoSecretsPlugin
{

public:
    FdoSecretsPluginGUI(DatabaseTabWidget* databases);
    ~FdoSecretsPluginGUI() override = default;

private slots:
    void registerDatabase(const QString& name, DatabaseWidget* dbWidget);
    void unregisterDatabase(const QString& name);

    void databaseTabOpened(DatabaseWidget* dbWidget);

private:
    size_t requestEntriesRemove(const QSharedPointer<FdoSecrets::DBusClient>& client,
                                const QString& name,
                                const QList<Entry*>& entries,
                                bool permanent) const override;

    bool requestEntriesUnlock(const QSharedPointer<FdoSecrets::DBusClient>& client,
                              const QString& windowId,
                              const QList<Entry*>& entries,
                              QHash<Entry*, AuthDecision>& decisions,
                              AuthDecision& forFutureEntries) const override;

    bool doLockDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client, const QString& name) override;
    bool doUnlockDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client, const QString& name) override;
    bool requestUnlockAnyDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client) const override;
    QString requestNewDatabase(const QSharedPointer<FdoSecrets::DBusClient>& client) override;
    QString overrideMessageBoxParent(const QString& windowId) const override;
    QWindow* findWindow(const QString& windowId) const;

    QString getDatabaseName(const QSharedPointer<Database>& db) const;
    void monitorDatabaseExposedGroup(DatabaseWidget* dbWidget);

private:
    QHash<QString, DatabaseWidget*> m_nameToWidget;
    DatabaseTabWidget* m_databases;
    QSet<const DatabaseWidget*> m_lockingDb; // list of dbs being locked
    QSet<const DatabaseWidget*> m_unlockingDb; // list of dbs being unlocked
};

#endif // KEEPASSXC_FDOSECRETSPLUGINGUI_H
