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

#ifndef KEEPASSXC_FDOSECRETS_COLLECTION_H
#define KEEPASSXC_FDOSECRETS_COLLECTION_H

#include "fdosecrets/dbus/DBusClient.h"
#include "fdosecrets/dbus/DBusObject.h"

#include "core/EntrySearcher.h"

class Database;
class Entry;
class FdoSecretsPlugin;
class Group;

namespace FdoSecrets
{
    class Item;
    class PromptBase;
    class Service;
    class Collection : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_COLLECTION_LITERAL)

        explicit Collection(Service* parent, const QString& name);

    public:
        /**
         * @brief Create a new instance of `Collection`
         * @param parent the owning Service
         * @param name the collection name
         * @return pointer to created instance, or nullptr when error happens.
         * This may be caused by
         *   - DBus path registration error
         */
        static Collection* Create(Service* parent, const QString& name);
        ~Collection() override;

        /**
         * D-Bus Properties
         */

        Q_INVOKABLE DBUS_PROPERTY DBusResult items(QList<Item*>& items) const;

        Q_INVOKABLE DBUS_PROPERTY DBusResult label(QString& label) const;
        Q_INVOKABLE DBusResult setLabel(const QString& label);

        Q_INVOKABLE DBUS_PROPERTY DBusResult locked(bool& locked) const;

        Q_INVOKABLE DBUS_PROPERTY DBusResult created(qulonglong& created) const;

        Q_INVOKABLE DBUS_PROPERTY DBusResult modified(qulonglong& modified) const;

        /**
         * D-Bus Methods
         */
        Q_INVOKABLE DBusResult remove(PromptBase*& prompt);
        Q_INVOKABLE DBusResult searchItems(const DBusClientPtr& client,
                                           const StringStringMap& attributes,
                                           QList<Item*>& items);
        Q_INVOKABLE DBusResult
        createItem(const QVariantMap& properties, const Secret& secret, bool replace, Item*& item, PromptBase*& prompt);

    signals:
        /**
         * D-Bus Signals
         */
        void itemCreated(Item* item);
        void itemDeleted(Item* item);
        void itemChanged(Item* item);

        void collectionChanged();
        void collectionAboutToDelete();
        void collectionLockChanged(bool newLocked);

    public:
        // For setting multiple properties at once
        DBusResult setProperties(const QVariantMap& properties);

        /**
         * A human readable name of the collection, available even if the db is locked
         * @return
         */
        QString name() const
        {
            return m_name;
        }

        QString dbusName() const;
        Group* exposedRootGroup() const;
        Database* backend() const
        {
            return m_backend.data();
        }

        // Access to ancestors
        Service* service() const
        {
            return qobject_cast<Service*>(parent());
        }
        FdoSecretsPlugin* plugin() const;

        static EntrySearcher::SearchTerm attributeToTerm(const QString& key, const QString& value);

    public slots:
        // expose some methods for Prompt to use
        bool doLock(const DBusClientPtr& client) const;
        bool doUnlock(const DBusClientPtr& client) const;

        bool doDeleteItem(const DBusClientPtr& client, const Item* item) const;

        Item* doNewItem(const DBusClientPtr& client, QString itemPath);

        // Only delete from dbus, will remove self. Do not affect database in KPXC
        void removeFromDBus();

        void reloadBackend();

        void databaseLocked();
        void databaseUnlocked(QSharedPointer<Database> db);

    private:
        friend class LockCollectionsPrompt;
        friend class UnlockPrompt;
        friend class DeleteCollectionPrompt;
        friend class CreateItemPrompt;

        Item* onEntryAdded(Entry* entry);
        void populateContents();
        void connectGroupSignalRecursive(Group* group);
        void cleanupConnections();

        /**
         * Check if the backend is a valid object, send error reply if not.
         * @return true if the backend is valid.
         */
        DBusResult ensureBackend() const;

        /**
         * Like mkdir -p, find or create the group by path, under m_exposedGroup
         * @param groupPath
         * @return created or found group
         */
        Group* findCreateGroupByPath(const QString& groupPath);

    private:
        QSharedPointer<Database> m_backend;

        QString m_name;

        QPointer<Group> m_exposedGroup;

        QList<Item*> m_items;
        QMap<const Entry*, Item*> m_entryToItem;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_COLLECTION_H
