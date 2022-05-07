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

#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/dbus/DBusObject.h"

class DatabaseTabWidget;
class DatabaseWidget;
class Group;

class FdoSecretsPlugin;

namespace FdoSecrets
{

    class Collection;
    class Item;
    class PromptBase;
    class Session;

    class Service : public DBusObject // clazy: exclude=ctor-missing-parent-argument
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_SERVICE_LITERAL)

        explicit Service(FdoSecretsPlugin* plugin, QPointer<DatabaseTabWidget> dbTabs, QSharedPointer<DBusMgr> dbus);

    public:
        /**
         * @brief Create a new instance of `Service`. Its parent is set to `null`
         * @return pointer to newly created Service, or nullptr if error
         * This may be caused by
         *   - failed initialization
         */
        static QSharedPointer<Service>
        Create(FdoSecretsPlugin* plugin, QPointer<DatabaseTabWidget> dbTabs, QSharedPointer<DBusMgr> dbus);
        ~Service() override;

        Q_INVOKABLE DBusResult openSession(const DBusClientPtr& client,
                                           const QString& algorithm,
                                           const QVariant& input,
                                           QVariant& output,
                                           Session*& result);
        Q_INVOKABLE DBusResult createCollection(const QVariantMap& properties,
                                                const QString& alias,
                                                Collection*& collection,
                                                PromptBase*& prompt);
        Q_INVOKABLE DBusResult searchItems(const DBusClientPtr& client,
                                           const StringStringMap& attributes,
                                           QList<Item*>& unlocked,
                                           QList<Item*>& locked);

        Q_INVOKABLE DBusResult unlock(const DBusClientPtr& client,
                                      const QList<DBusObject*>& objects,
                                      QList<DBusObject*>& unlocked,
                                      PromptBase*& prompt);

        Q_INVOKABLE DBusResult lock(const QList<DBusObject*>& objects, QList<DBusObject*>& locked, PromptBase*& prompt);

        Q_INVOKABLE DBusResult getSecrets(const DBusClientPtr& client,
                                          const QList<Item*>& items,
                                          Session* session,
                                          ItemSecretMap& secrets) const;

        Q_INVOKABLE DBusResult readAlias(const QString& name, Collection*& collection) const;

        Q_INVOKABLE DBusResult setAlias(const QString& name, Collection* collection);

        /**
         * List of collections
         * @return
         */
        Q_INVOKABLE DBUS_PROPERTY DBusResult collections(QList<Collection*>& collections) const;

    signals:
        void collectionCreated(Collection* collection);
        void collectionDeleted(Collection* collection);
        void collectionChanged(Collection* collection);

        /**
         * Finish signal for async action doUnlockDatabaseInDialog
         * @param accepted If false, the action is cancelled by the user
         * @param dbWidget The dbWidget the action is on
         */
        void doneUnlockDatabaseInDialog(bool accepted, DatabaseWidget* dbWidget);

    public:
        /**
         * List of sessions
         * @return
         */
        QList<Session*> sessions() const;

        FdoSecretsPlugin* plugin() const
        {
            return m_plugin;
        }

    public slots:
        bool doLockDatabase(DatabaseWidget* dbWidget);
        bool doCloseDatabase(DatabaseWidget* dbWidget);
        Collection* doNewDatabase();
        void doSwitchToDatabaseSettings(DatabaseWidget* dbWidget);

        /**
         * Async, connect to signal doneUnlockDatabaseInDialog for finish notification
         * @param dbWidget
         */
        void doUnlockDatabaseInDialog(DatabaseWidget* dbWidget);

        /**
         * Async, connect to signal doneUnlockDatabaseInDialog for finish notification
         * @param dbWidget
         */
        void doUnlockAnyDatabaseInDialog();

    private slots:
        void ensureDefaultAlias();

        void onDatabaseUnlockDialogFinished(bool accepted, DatabaseWidget* dbWidget);
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

        DBusResult unlockedCollections(QList<Collection*>& unlocked) const;

    private:
        FdoSecretsPlugin* m_plugin{nullptr};
        QPointer<DatabaseTabWidget> m_databases{};

        QHash<QString, Collection*> m_aliases{};
        QList<Collection*> m_collections{};
        QHash<const DatabaseWidget*, Collection*> m_dbToCollection{};

        QList<Session*> m_sessions{};

        bool m_insideEnsureDefaultAlias{false};
        bool m_unlockingAnyDatabase{false};
        // list of db currently has unlock dialog shown
        QHash<const DatabaseWidget*, QMetaObject::Connection> m_unlockingDb{};
        QSet<const DatabaseWidget*> m_lockingDb{}; // list of db being locking
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SERVICE_H
