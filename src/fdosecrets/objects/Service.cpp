/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "Service.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Session.h"

#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QSharedPointer>

namespace
{
    constexpr auto DEFAULT_ALIAS = "default";
}

namespace FdoSecrets
{
    QSharedPointer<Service> Service::Create(FdoSecretsPlugin* plugin, QPointer<DatabaseTabWidget> dbTabs)
    {
        QSharedPointer<Service> res{new Service(plugin, std::move(dbTabs))};
        if (!res->initialize()) {
            return {};
        }
        return res;
    }

    Service::Service(FdoSecretsPlugin* plugin,
                     QPointer<DatabaseTabWidget> dbTabs) // clazy: exclude=ctor-missing-parent-argument
        : DBusObjectHelper(nullptr)
        , m_plugin(plugin)
        , m_databases(std::move(dbTabs))
        , m_insideEnsureDefaultAlias(false)
        , m_serviceWatcher(nullptr)
    {
        connect(
            m_databases, &DatabaseTabWidget::databaseUnlockDialogFinished, this, &Service::doneUnlockDatabaseInDialog);
    }

    Service::~Service()
    {
        QDBusConnection::sessionBus().unregisterService(QStringLiteral(DBUS_SERVICE_SECRET));
    }

    bool Service::initialize()
    {
        if (!QDBusConnection::sessionBus().registerService(QStringLiteral(DBUS_SERVICE_SECRET))) {
            plugin()->emitError(
                tr("Failed to register DBus service at %1.<br/>").arg(QLatin1String(DBUS_SERVICE_SECRET))
                + m_plugin->reportExistingService());
            return false;
        }

        if (!registerWithPath(QStringLiteral(DBUS_PATH_SECRETS))) {
            plugin()->emitError(tr("Failed to register DBus path %1.<br/>").arg(QStringLiteral(DBUS_PATH_SECRETS)));
            return false;
        }

        // Connect to service unregistered signal
        m_serviceWatcher.reset(new QDBusServiceWatcher());
        connect(
            m_serviceWatcher.get(), &QDBusServiceWatcher::serviceUnregistered, this, &Service::dbusServiceUnregistered);

        m_serviceWatcher->setConnection(QDBusConnection::sessionBus());

        // Add existing database tabs
        for (int idx = 0; idx != m_databases->count(); ++idx) {
            auto dbWidget = m_databases->databaseWidgetFromIndex(idx);
            onDatabaseTabOpened(dbWidget, false);
        }

        // Connect to new database signal
        // No need to connect to close signal, as collection will remove itself when backend delete/close database tab.
        connect(m_databases.data(), &DatabaseTabWidget::databaseOpened, this, [this](DatabaseWidget* dbWidget) {
            onDatabaseTabOpened(dbWidget, true);
        });

        // make default alias track current activated database
        connect(m_databases.data(), &DatabaseTabWidget::activateDatabaseChanged, this, &Service::ensureDefaultAlias);

        return true;
    }

    void Service::onDatabaseTabOpened(DatabaseWidget* dbWidget, bool emitSignal)
    {
        // The Collection will monitor the database's exposed group.
        // When the Collection finds that no exposed group, it will delete itself.
        // Thus the service also needs to monitor it and recreate the collection if the user changes
        // from no exposed to exposed something.
        connect(dbWidget, &DatabaseWidget::databaseReplaced, this, [this, dbWidget]() {
            monitorDatabaseExposedGroup(dbWidget);
        });
        if (!dbWidget->isLocked()) {
            monitorDatabaseExposedGroup(dbWidget);
        }

        auto coll = Collection::Create(this, dbWidget);
        if (!coll) {
            return;
        }

        m_collections << coll;
        m_dbToCollection[dbWidget] = coll;

        // keep record of the collection existence
        connect(coll, &Collection::collectionAboutToDelete, this, [this, coll]() {
            m_collections.removeAll(coll);
            m_dbToCollection.remove(coll->backend());
        });

        // keep record of alias
        connect(coll, &Collection::aliasAboutToAdd, this, &Service::onCollectionAliasAboutToAdd);
        connect(coll, &Collection::aliasAdded, this, &Service::onCollectionAliasAdded);
        connect(coll, &Collection::aliasRemoved, this, &Service::onCollectionAliasRemoved);

        // Forward delete signal, we have to rely on filepath to identify the database being closed,
        // but we can not access m_backend safely because during the databaseClosed signal,
        // m_backend may already be reset to nullptr
        // We want to remove the collection object from dbus as early as possible, to avoid
        // race conditions when deleteLater was called on the m_backend, but not delivered yet,
        // and new method calls from dbus occurred. Therefore we can't rely on the destroyed
        // signal on m_backend.
        // bind to coll lifespan
        connect(m_databases.data(), &DatabaseTabWidget::databaseClosed, coll, [coll](const QString& filePath) {
            if (filePath == coll->backendFilePath()) {
                coll->doDelete();
            }
        });

        // actual load, must after updates to m_collections, because the reload may trigger
        // another onDatabaseTabOpen, and m_collections will be used to prevent recursion.
        if (!coll->reloadBackend()) {
            // error in dbus
            return;
        }
        if (!coll->backend()) {
            // no exposed group on this db
            return;
        }

        ensureDefaultAlias();

        // only start relay signals when the collection is fully setup
        connect(coll, &Collection::collectionChanged, this, [this, coll]() { emit collectionChanged(coll); });
        connect(coll, &Collection::collectionAboutToDelete, this, [this, coll]() { emit collectionDeleted(coll); });
        if (emitSignal) {
            emit collectionCreated(coll);
        }
    }

    void Service::monitorDatabaseExposedGroup(DatabaseWidget* dbWidget)
    {
        Q_ASSERT(dbWidget);
        connect(
            dbWidget->database()->metadata()->customData(), &CustomData::customDataModified, this, [this, dbWidget]() {
                if (!FdoSecrets::settings()->exposedGroup(dbWidget->database()).isNull() && !findCollection(dbWidget)) {
                    onDatabaseTabOpened(dbWidget, true);
                }
            });
    }

    void Service::ensureDefaultAlias()
    {
        if (m_insideEnsureDefaultAlias) {
            return;
        }

        m_insideEnsureDefaultAlias = true;

        auto coll = findCollection(m_databases->currentDatabaseWidget());
        if (coll) {
            // adding alias will automatically remove the association with previous collection.
            coll->addAlias(DEFAULT_ALIAS).okOrDie();
        }

        m_insideEnsureDefaultAlias = false;
    }

    void Service::dbusServiceUnregistered(const QString& service)
    {
        Q_ASSERT(m_serviceWatcher);

        auto removed = m_serviceWatcher->removeWatchedService(service);
        if (!removed) {
            qDebug("FdoSecrets: Failed to remove service watcher");
        }

        Session::CleanupNegotiation(service);
        auto sess = m_peerToSession.value(service, nullptr);
        if (sess) {
            sess->close().okOrDie();
        }
    }

    DBusReturn<const QList<Collection*>> Service::collections() const
    {
        return m_collections;
    }

    DBusReturn<QVariant> Service::openSession(const QString& algorithm, const QVariant& input, Session*& result)
    {
        QVariant output;
        bool incomplete = false;
        auto peer = callingPeer();

        // watch for service unregister to cleanup
        Q_ASSERT(m_serviceWatcher);
        m_serviceWatcher->addWatchedService(peer);

        // negotiate cipher
        auto ciphers = Session::CreateCiphers(peer, algorithm, input, output, incomplete);
        if (incomplete) {
            result = nullptr;
            return output;
        }
        if (!ciphers) {
            return DBusReturn<>::Error(QDBusError::NotSupported);
        }
        result = Session::Create(std::move(ciphers), callingPeerName(), this);
        if (!result) {
            return DBusReturn<>::Error(QDBusError::InvalidObjectPath);
        }

        m_sessions.append(result);
        m_peerToSession[peer] = result;
        connect(result, &Session::aboutToClose, this, [this, peer, result]() {
            emit sessionClosed(result);
            m_sessions.removeAll(result);
            m_peerToSession.remove(peer);
        });
        emit sessionOpened(result);

        return output;
    }

    DBusReturn<Collection*>
    Service::createCollection(const QVariantMap& properties, const QString& alias, PromptBase*& prompt)
    {
        prompt = nullptr;

        // return existing collection if alias is non-empty and exists.
        auto collection = findCollection(alias);
        if (!collection) {
            auto cp = CreateCollectionPrompt::Create(this);
            if (cp.isError()) {
                return cp;
            }
            prompt = cp.value();

            // collection will be created when the prompt completes.
            // once it's done, we set additional properties on the collection
            connect(cp.value(),
                    &CreateCollectionPrompt::collectionCreated,
                    cp.value(),
                    [alias, properties](Collection* coll) {
                        coll->setProperties(properties).okOrDie();
                        if (!alias.isEmpty()) {
                            coll->addAlias(alias).okOrDie();
                        }
                    });
        }
        return collection;
    }

    DBusReturn<const QList<Item*>> Service::searchItems(const StringStringMap& attributes, QList<Item*>& locked)
    {
        auto ret = collections();
        if (ret.isError()) {
            return ret;
        }

        QList<Item*> unlocked;
        for (const auto& coll : ret.value()) {
            auto items = coll->searchItems(attributes);
            if (items.isError()) {
                return items;
            }
            auto l = coll->locked();
            if (l.isError()) {
                return l;
            }
            if (l.value()) {
                locked.append(items.value());
            } else {
                unlocked.append(items.value());
            }
        }
        return unlocked;
    }

    DBusReturn<const QList<DBusObject*>> Service::unlock(const QList<DBusObject*>& objects, PromptBase*& prompt)
    {
        QSet<Collection*> needUnlock;
        needUnlock.reserve(objects.size());
        for (const auto& obj : asConst(objects)) {
            auto coll = qobject_cast<Collection*>(obj);
            if (coll) {
                needUnlock << coll;
            } else {
                auto item = qobject_cast<Item*>(obj);
                if (!item) {
                    continue;
                }
                // we lock the whole collection for item
                needUnlock << item->collection();
            }
        }

        // return anything already unlocked
        QList<DBusObject*> unlocked;
        QList<Collection*> toUnlock;
        for (const auto& coll : asConst(needUnlock)) {
            auto l = coll->locked();
            if (l.isError()) {
                return l;
            }
            if (!l.value()) {
                unlocked << coll;
            } else {
                toUnlock << coll;
            }
        }
        if (!toUnlock.isEmpty()) {
            auto up = UnlockCollectionsPrompt::Create(this, toUnlock);
            if (up.isError()) {
                return up;
            }
            prompt = up.value();
        }
        return unlocked;
    }

    DBusReturn<const QList<DBusObject*>> Service::lock(const QList<DBusObject*>& objects, PromptBase*& prompt)
    {
        QSet<Collection*> needLock;
        needLock.reserve(objects.size());
        for (const auto& obj : asConst(objects)) {
            auto coll = qobject_cast<Collection*>(obj);
            if (coll) {
                needLock << coll;
            } else {
                auto item = qobject_cast<Item*>(obj);
                if (!item) {
                    continue;
                }
                // we lock the whole collection for item
                needLock << item->collection();
            }
        }

        // return anything already locked
        QList<DBusObject*> locked;
        QList<Collection*> toLock;
        for (const auto& coll : asConst(needLock)) {
            auto l = coll->locked();
            if (l.isError()) {
                return l;
            }
            if (l.value()) {
                locked << coll;
            } else {
                toLock << coll;
            }
        }
        if (!toLock.isEmpty()) {
            auto lp = LockCollectionsPrompt::Create(this, toLock);
            if (lp.isError()) {
                return lp;
            }
            prompt = lp.value();
        }
        return locked;
    }

    DBusReturn<const QHash<Item*, SecretStruct>> Service::getSecrets(const QList<Item*>& items, Session* session)
    {
        if (!session) {
            return DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SESSION));
        }

        QHash<Item*, SecretStruct> res;

        for (const auto& item : asConst(items)) {
            auto ret = item->getSecret(session);
            if (ret.isError()) {
                return ret;
            }
            res[item] = std::move(ret).value();
        }
        if (calledFromDBus()) {
            plugin()->emitRequestShowNotification(
                tr(R"(%n Entry(s) was used by %1)", "%1 is the name of an application", res.size())
                    .arg(callingPeerName()));
        }
        return res;
    }

    DBusReturn<Collection*> Service::readAlias(const QString& name)
    {
        return findCollection(name);
    }

    DBusReturn<void> Service::setAlias(const QString& name, Collection* collection)
    {
        if (!collection) {
            // remove alias name from its collection
            collection = findCollection(name);
            if (!collection) {
                return DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT));
            }
            return collection->removeAlias(name);
        }
        return collection->addAlias(name);
    }

    Collection* Service::findCollection(const QString& alias) const
    {
        if (alias.isEmpty()) {
            return nullptr;
        }

        auto it = m_aliases.find(alias);
        if (it != m_aliases.end()) {
            return it.value();
        }
        return nullptr;
    }

    void Service::onCollectionAliasAboutToAdd(const QString& alias)
    {
        auto coll = qobject_cast<Collection*>(sender());

        auto it = m_aliases.constFind(alias);
        if (it != m_aliases.constEnd() && it.value() != coll) {
            // another collection holds the alias
            // remove it first
            it.value()->removeAlias(alias).okOrDie();

            // onCollectionAliasRemoved called through signal
            // `it` becomes invalidated now
        }
    }

    void Service::onCollectionAliasAdded(const QString& alias)
    {
        auto coll = qobject_cast<Collection*>(sender());
        m_aliases[alias] = coll;
    }

    void Service::onCollectionAliasRemoved(const QString& alias)
    {
        m_aliases.remove(alias);
        ensureDefaultAlias();
    }

    Collection* Service::findCollection(const DatabaseWidget* db) const
    {
        return m_dbToCollection.value(db, nullptr);
    }

    const QList<Session*> Service::sessions() const
    {
        return m_sessions;
    }

    bool Service::doCloseDatabase(DatabaseWidget* dbWidget)
    {
        return m_databases->closeDatabaseTab(dbWidget);
    }

    Collection* Service::doNewDatabase()
    {
        auto dbWidget = m_databases->newDatabase();
        if (!dbWidget) {
            return nullptr;
        }

        // database created through dbus will be exposed to dbus by default
        auto db = dbWidget->database();
        FdoSecrets::settings()->setExposedGroup(db, db->rootGroup()->uuid());

        auto collection = findCollection(dbWidget);

        Q_ASSERT(collection);

        return collection;
    }

    void Service::doSwitchToDatabaseSettings(DatabaseWidget* dbWidget)
    {
        if (dbWidget->isLocked()) {
            return;
        }
        // switch selected to current
        m_databases->setCurrentWidget(dbWidget);
        m_databases->showDatabaseSettings();

        // open settings (switch from app settings to m_dbTabs)
        m_plugin->emitRequestSwitchToDatabases();
    }

    void Service::doUnlockDatabaseInDialog(DatabaseWidget* dbWidget)
    {
        m_databases->unlockDatabaseInDialog(dbWidget, DatabaseOpenDialog::Intent::None);
    }

} // namespace FdoSecrets
