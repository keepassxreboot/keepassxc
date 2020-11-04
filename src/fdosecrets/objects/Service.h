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

#ifndef KEEPASSXC_FDOSECRETS_SERVICE_H
#define KEEPASSXC_FDOSECRETS_SERVICE_H

#include "fdosecrets/objects/DBusObject.h"
#include "fdosecrets/objects/adaptors/ServiceAdaptor.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariant>

#include <memory>

class QDBusServiceWatcher;

class DatabaseTabWidget;
class DatabaseWidget;
class Group;

class FdoSecretsPlugin;

namespace FdoSecrets
{

    class Collection;
    class Item;
    class PromptBase;
    class ServiceAdaptor;
    class Session;

    class Service : public DBusObjectHelper<Service, ServiceAdaptor> // clazy: exclude=ctor-missing-parent-argument
    {
        Q_OBJECT

        explicit Service(FdoSecretsPlugin* plugin, QPointer<DatabaseTabWidget> dbTabs);

    public:
        /**
         * @brief Create a new instance of `Service`. Its parent is set to `null`
         * @return pointer to newly created Service, or nullptr if error
         * This may be caused by
         *   - failed initialization
         */
        static QSharedPointer<Service> Create(FdoSecretsPlugin* plugin, QPointer<DatabaseTabWidget> dbTabs);
        ~Service() override;

        DBusReturn<QVariant> openSession(const QString& algorithm, const QVariant& input, Session*& result);
        DBusReturn<Collection*>
        createCollection(const QVariantMap& properties, const QString& alias, PromptBase*& prompt);
        DBusReturn<const QList<Item*>> searchItems(const StringStringMap& attributes, QList<Item*>& locked);

        DBusReturn<const QList<DBusObject*>> unlock(const QList<DBusObject*>& objects, PromptBase*& prompt);

        DBusReturn<const QList<DBusObject*>> lock(const QList<DBusObject*>& objects, PromptBase*& prompt);

        DBusReturn<const QHash<Item*, SecretStruct>> getSecrets(const QList<Item*>& items, Session* session);

        DBusReturn<Collection*> readAlias(const QString& name);

        DBusReturn<void> setAlias(const QString& name, Collection* collection);

        /**
         * List of collections
         * @return
         */
        DBusReturn<const QList<Collection*>> collections() const;

    signals:
        void collectionCreated(Collection* collection);
        void collectionDeleted(Collection* collection);
        void collectionChanged(Collection* collection);

        void sessionOpened(Session* sess);
        void sessionClosed(Session* sess);

        /**
         * Finish signal for async action doUnlockDatabaseInDialog
         * @param accepted If false, the action is canceled by the user
         * @param dbWidget The unlocked the dbWidget if succeed
         */
        void doneUnlockDatabaseInDialog(bool accepted, DatabaseWidget* dbWidget);

    public:
        /**
         * List of sessions
         * @return
         */
        const QList<Session*> sessions() const;

        FdoSecretsPlugin* plugin() const
        {
            return m_plugin;
        }

    public slots:
        bool doCloseDatabase(DatabaseWidget* dbWidget);
        Collection* doNewDatabase();
        void doSwitchToDatabaseSettings(DatabaseWidget* dbWidget);

        /**
         * Async, connect to signal doneUnlockDatabaseInDialog for finish notification
         * @param dbWidget
         */
        void doUnlockDatabaseInDialog(DatabaseWidget* dbWidget);

    private slots:
        void dbusServiceUnregistered(const QString& service);
        void ensureDefaultAlias();

        void onDatabaseTabOpened(DatabaseWidget* dbWidget, bool emitSignal);
        void monitorDatabaseExposedGroup(DatabaseWidget* dbWidget);

        void onCollectionAliasAboutToAdd(const QString& alias);
        void onCollectionAliasAdded(const QString& alias);

        void onCollectionAliasRemoved(const QString& alias);

    private:
        bool initialize();

        /**
         * Find collection by alias name
         * @param alias
         * @return the collection under alias
         */
        Collection* findCollection(const QString& alias) const;

        /**
         * Find collection by dbWidget
         * @param db
         * @return the collection corresponding to the db
         */
        Collection* findCollection(const DatabaseWidget* db) const;

    private:
        FdoSecretsPlugin* m_plugin;
        QPointer<DatabaseTabWidget> m_databases;

        QHash<QString, Collection*> m_aliases;
        QList<Collection*> m_collections;
        QHash<const DatabaseWidget*, Collection*> m_dbToCollection;

        QList<Session*> m_sessions;
        QHash<QString, Session*> m_peerToSession;

        bool m_insideEnsureDefaultAlias;

        std::unique_ptr<QDBusServiceWatcher> m_serviceWatcher;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SERVICE_H
