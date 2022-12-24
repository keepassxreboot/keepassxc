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

class Database;
class Entry;
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

        explicit Service(FdoSecretsPlugin* plugin, QSharedPointer<DBusMgr> dbus);

    public:
        /**
         * @brief Create a new instance of `Service`. Its parent is set to `null`
         * @return pointer to newly created Service, or nullptr if error
         * This may be caused by
         *   - failed initialization
         */
        static QSharedPointer<Service> Create(FdoSecretsPlugin* plugin, QSharedPointer<DBusMgr> dbus);
        ~Service() override;

        /**
         * D-Bus Methods
         */

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
         * D-Bus Properties
         */

        /**
         * List of collections
         * @return
         */
        Q_INVOKABLE DBUS_PROPERTY DBusResult collections(QList<Collection*>& collections) const;

    signals:
        /**
         * D-Bus Signals
         */

        void collectionCreated(Collection* collection);
        void collectionDeleted(Collection* collection);
        void collectionChanged(Collection* collection);

    public:
        /**
         * List of sessions
         * @return
         */
        QList<Session*> sessions() const;

        bool setAlias(const QString& alias, const QString& name);
        QStringList collectionAliases(const Collection* collection) const;

        // Access to ancestors
        FdoSecretsPlugin* plugin() const
        {
            return m_plugin;
        }

    public slots:
        Collection* doNewDatabase(const DBusClientPtr& client);

        void registerDatabase(const QString& name);
        void unregisterDatabase(const QString& name);
        void databaseLocked(const QString& name);
        void databaseUnlocked(const QString& name, QSharedPointer<Database> db);

    private slots:
        void ensureDefaultAlias();

    private:
        friend class CreateCollectionPrompt;

        DBusResult unlockedCollections(QList<Collection*>& unlocked) const;

        Collection* createCollection(const QString& name);

    private:
        FdoSecretsPlugin* m_plugin{nullptr};

        QHash<QString, Collection*> m_aliasToCollection{};
        QHash<QString, Collection*> m_nameToCollection{};
        QList<Collection*> m_collections{};

        QList<Session*> m_sessions{};
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SERVICE_H
