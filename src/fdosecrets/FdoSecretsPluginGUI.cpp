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

#include "FdoSecretsPluginGUI.h"

#include <QWindow>

#include "core/Entry.h"
#include "core/Group.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/widgets/AccessControlDialog.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/GuiTools.h"
#include "gui/MessageBox.h"

FdoSecretsPluginGUI::FdoSecretsPluginGUI(DatabaseTabWidget* databases)
    : FdoSecretsPlugin{databases}
    , m_databases{databases}
    , m_lockingDb{}
    , m_unlockingDb{}
{
    // Don't connect signals until service is enabled
    connect(this, &FdoSecretsPlugin::secretServiceStarted, this, [this]() {
        // Connect to new database signal
        connect(m_databases, &DatabaseTabWidget::databaseOpened, this, &FdoSecretsPluginGUI::databaseTabOpened);

        // Populate/Clear the database when lock state changes
        connect(m_databases, &DatabaseTabWidget::databaseUnlocked, this, [this](DatabaseWidget* dbWidget) {
            auto db = dbWidget->database();
            auto name = getDatabaseName(db);
            if (FdoSecrets::settings()->exposedGroup(db.data()).isNull()) {
                return;
            }

            Q_ASSERT(m_nameToWidget.value(name) == dbWidget);
            databaseUnlocked(name, db);
        });

        connect(m_databases, &DatabaseTabWidget::databaseLocked, this, [this](DatabaseWidget* dbWidget) {
            databaseLocked(getDatabaseName(dbWidget->database()));
        });

        // Make default alias track current activated database
        connect(m_databases, &DatabaseTabWidget::activeDatabaseChanged, this, [this](DatabaseWidget* dbWidget) {
            if (dbWidget) {
                auto db = dbWidget->database();
                if (FdoSecrets::settings()->exposedGroup(db.data()).isNull()) {
                    return;
                }

                serviceInstance()->setAlias("default", getDatabaseName(db));
            }
        });

        // Clear the local state when collection deletes
        connect(serviceInstance(), &FdoSecrets::Service::collectionDeleted, this, [this](FdoSecrets::Collection* coll) {
            if (!FdoSecrets::settings()->exposedGroup(coll->backend()).isNull()) {
                m_databases->closeDatabaseTab(m_nameToWidget.value(coll->name()));
            }

            m_nameToWidget.remove(coll->name());
        });

        // Add existing database tabs
        for (int idx = 0; idx < m_databases->count(); ++idx) {
            auto dbWidget = m_databases->databaseWidgetFromIndex(idx);
            databaseTabOpened(dbWidget);
        }
    });

    // Clear local state when service stops
    connect(this, &FdoSecretsPlugin::secretServiceStopped, this, [this]() {
        m_databases->disconnect(this);
        for (int idx = 0; idx < m_databases->count(); ++idx) {
            m_databases->databaseWidgetFromIndex(idx)->disconnect(this);
        }

        m_nameToWidget.clear();
    });
}

void FdoSecretsPluginGUI::registerDatabase(const QString& name, DatabaseWidget* dbWidget)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(m_nameToWidget.value(name) == nullptr);
    m_nameToWidget[name] = dbWidget;
    FdoSecretsPlugin::registerDatabase(name);

    // If the db is already unlocked we won't get additional signal
    if (!dbWidget->isLocked()) {
        databaseUnlocked(name, dbWidget->database());
    }
}

void FdoSecretsPluginGUI::unregisterDatabase(const QString& name)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(m_nameToWidget.value(name));
    FdoSecretsPlugin::unregisterDatabase(name);
    m_nameToWidget.remove(name);
}

void FdoSecretsPluginGUI::databaseTabOpened(DatabaseWidget* dbWidget)
{
    // The Collection will monitor the database's exposed group.
    // When the Collection finds that no exposed group, it will delete itself.
    // Thus, the plugin also needs to monitor it and recreate the collection if the user changes
    // from no exposed to exposed something.
    connect(dbWidget, &DatabaseWidget::databaseReplaced, this, [this, dbWidget]() {
        monitorDatabaseExposedGroup(dbWidget);
    });

    if (!dbWidget->isLocked()) {
        monitorDatabaseExposedGroup(dbWidget);
    }

    // No exposed group, no point in registering as it will remove itself immediately anyway
    if (!dbWidget->isLocked() && FdoSecrets::settings()->exposedGroup(dbWidget->database().data()).isNull()) {
        return;
    }

    registerDatabase(getDatabaseName(dbWidget->database()), dbWidget);

    // Reload Collection when database is replaced
    connect(dbWidget,
            &DatabaseWidget::databaseReplaced,
            this,
            [this, dbWidget](QSharedPointer<Database> oldDb, QSharedPointer<Database> newDb) {
                Q_ASSERT(oldDb != newDb);
                auto oldName = getDatabaseName(oldDb);
                auto newName = getDatabaseName(newDb);
                if (oldName != newName) {
                    unregisterDatabase(oldName);
                    registerDatabase(newName, dbWidget);
                } else {
                    // If the db is already unlocked we won't get additional signal
                    if (!dbWidget->isLocked()) {
                        databaseUnlocked(newName, dbWidget->database());
                    }
                }
            });

    // Unregister on close
    connect(dbWidget, &DatabaseWidget::closeRequest, this, [this, dbWidget]() {
        unregisterDatabase(getDatabaseName(dbWidget->database()));
    });

    // Reload Collection on file path changed
    connect(dbWidget,
            &DatabaseWidget::databaseFilePathChanged,
            this,
            [this, dbWidget](const QString& oldPath, const QString&) {
                unregisterDatabase(QFileInfo(oldPath).completeBaseName());
                registerDatabase(getDatabaseName(dbWidget->database()), dbWidget);
            });
}

size_t FdoSecretsPluginGUI::requestEntriesRemove(const QSharedPointer<FdoSecrets::DBusClient>&,
                                                 const QString& name,
                                                 const QList<Entry*>& entries,
                                                 bool permanent) const
{
    DatabaseWidget* dbWidget = m_nameToWidget.value(name);
    if (!dbWidget) {
        return 0;
    }

    if (FdoSecrets::settings()->confirmDeleteItem() && !GuiTools::confirmDeleteEntries(dbWidget, entries, permanent)) {
        return 0;
    }

    return GuiTools::deleteEntriesResolveReferences(dbWidget, entries, permanent);
}

bool FdoSecretsPluginGUI::requestEntriesUnlock(const QSharedPointer<FdoSecrets::DBusClient>& client,
                                               const QString& windowId,
                                               const QList<Entry*>& entries,
                                               QHash<Entry*, AuthDecision>& decisions,
                                               AuthDecision& forFutureEntries) const
{
    QString app = QObject::tr("%1 (PID: %2)").arg(client->name()).arg(client->pid());
    auto ac = new AccessControlDialog(findWindow(windowId), entries, app, client->processInfo(), AuthOption::Remember);
    QObject::connect(ac,
                     &AccessControlDialog::finished,
                     [&](const QHash<Entry*, AuthDecision>& dialogDecisions, AuthDecision dialogForFutureEntries) {
                         decisions = dialogDecisions;
                         forFutureEntries = dialogForFutureEntries;
                     });
    ac->exec();
    ac->deleteLater();
    return true;
}

bool FdoSecretsPluginGUI::doLockDatabase(const QSharedPointer<FdoSecrets::DBusClient>&, const QString& name)
{
    DatabaseWidget* dbWidget = m_nameToWidget.value(name);
    if (!dbWidget) {
        return false;
    }

    // return immediately if the db is already locked
    if (dbWidget->isLocked()) {
        return true;
    }

    // Prevent multiple dialogs on the same database
    if (m_lockingDb.contains(dbWidget)) {
        return true;
    }

    m_lockingDb.insert(dbWidget);
    bool ret = dbWidget->lock();
    m_lockingDb.remove(dbWidget);
    return ret;
}

bool FdoSecretsPluginGUI::doUnlockDatabase(const QSharedPointer<FdoSecrets::DBusClient>&, const QString& name)
{
    DatabaseWidget* dbWidget = m_nameToWidget.value(name);
    if (!dbWidget) {
        return false;
    }

    // return immediately if the db is already unlocked
    if (!dbWidget->isLocked()) {
        return true;
    }

    // Prevent multiple dialogs on the same database
    if (!m_unlockingDb.contains(dbWidget)) {
        m_unlockingDb.insert(dbWidget);
        m_databases->unlockDatabaseInDialog(dbWidget, DatabaseOpenDialog::Intent::None);
    }

    QEventLoop loop;
    bool wasAccepted = false;
    QObject::connect(m_databases,
                     &DatabaseTabWidget::databaseUnlockDialogFinished,
                     &loop,
                     [&](bool accepted, DatabaseWidget* unlocked) {
                         if (unlocked == dbWidget) {
                             wasAccepted = accepted;
                             m_unlockingDb.remove(dbWidget);
                             loop.quit();
                         }
                     });

    loop.exec();
    return wasAccepted;
}

bool FdoSecretsPluginGUI::requestUnlockAnyDatabase(const QSharedPointer<FdoSecrets::DBusClient>&) const
{
    m_databases->unlockAnyDatabaseInDialog(DatabaseOpenDialog::Intent::None);
    QEventLoop loop;
    bool wasAccepted = false;
    QObject::connect(m_databases, &DatabaseTabWidget::databaseUnlockDialogFinished, &loop, [&](bool accepted) {
        wasAccepted = accepted;
        loop.quit();
    });

    loop.exec();
    return wasAccepted;
}

QString FdoSecretsPluginGUI::requestNewDatabase(const QSharedPointer<FdoSecrets::DBusClient>&)
{
    auto dbWidget = m_databases->newDatabase();
    if (!dbWidget) {
        return {};
    }

    // database created through D-Bus will be exposed to D-Bus by default
    auto db = dbWidget->database();
    FdoSecrets::settings()->setExposedGroup(db.data(), db->rootGroup()->uuid());
    return getDatabaseName(db);
}

QString FdoSecretsPluginGUI::overrideMessageBoxParent(const QString& windowId) const
{
    QString oldParentWindowId = {};

    if (MessageBox::m_overrideParent && MessageBox::m_overrideParent->winId()) {
        oldParentWindowId = QString::number(MessageBox::m_overrideParent->winId());
    }

    MessageBox::m_overrideParent = findWindow(windowId);
    return oldParentWindowId;
}

QWindow* FdoSecretsPluginGUI::findWindow(const QString& windowId) const
{
    // find parent window, or nullptr if not found
    bool ok = false;
    WId wid = windowId.toULongLong(&ok, 0);
    QWindow* parent = nullptr;
    if (ok) {
        parent = QWindow::fromWinId(wid);
    }
    if (parent) {
        // parent is not the child of any object, so make sure it gets deleted at some point
        QObject::connect(this, &QObject::destroyed, parent, &QObject::deleteLater);
    }
    return parent;
}

QString FdoSecretsPluginGUI::getDatabaseName(const QSharedPointer<Database>& db) const
{
    if (db->canonicalFilePath().isEmpty()) {
        // This is a newly created db without saving to file,
        // but we have to give a name, which is used to register D-Bus path.
        // We use database name for this purpose. For simplicity, we don't monitor the name change.
        // So the D-Bus object path is not updated if the db name changes.
        // This should not be a problem because once the database gets saved,
        // the D-Bus path will be updated to use filename and everything back to normal.
        return db->metadata()->name();
    }

    return db->canonicalFilePath();
}

void FdoSecretsPluginGUI::monitorDatabaseExposedGroup(DatabaseWidget* dbWidget)
{
    Q_ASSERT(dbWidget->database());
    connect(dbWidget->database()->metadata()->customData(), &CustomData::modified, this, [this, dbWidget]() {
        if (!FdoSecrets::settings()->exposedGroup(dbWidget->database().data()).isNull()
            && !m_nameToWidget.value(getDatabaseName(dbWidget->database()))) {
            databaseTabOpened(dbWidget);
        }
    });
}
