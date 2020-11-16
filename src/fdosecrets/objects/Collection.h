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

#include "DBusObject.h"

#include "adaptors/CollectionAdaptor.h"
#include "core/EntrySearcher.h"

#include <QPointer>
#include <QSet>

class Database;
class DatabaseWidget;
class Entry;
class Group;

namespace FdoSecrets
{
    class Item;
    class PromptBase;
    class Service;
    class Collection : public DBusObjectHelper<Collection, CollectionAdaptor>
    {
        Q_OBJECT

        explicit Collection(Service* parent, DatabaseWidget* backend);

    public:
        /**
         * @brief Create a new instance of `Collection`
         * @param parent the owning Service
         * @param backend the widget containing the database
         * @return pointer to created instance, or nullptr when error happens.
         * This may be caused by
         *   - DBus path registration error
         *   - database has no exposed group
         */
        static Collection* Create(Service* parent, DatabaseWidget* backend);

        DBusReturn<const QList<Item*>> items() const;

        DBusReturn<QString> label() const;
        DBusReturn<void> setLabel(const QString& label);

        DBusReturn<bool> locked() const;

        DBusReturn<qulonglong> created() const;

        DBusReturn<qulonglong> modified() const;

        DBusReturn<PromptBase*> deleteCollection();
        DBusReturn<const QList<Item*>> searchItems(const StringStringMap& attributes);
        DBusReturn<Item*>
        createItem(const QVariantMap& properties, const SecretStruct& secret, bool replace, PromptBase*& prompt);

    signals:
        void itemCreated(Item* item);
        void itemDeleted(Item* item);
        void itemChanged(Item* item);

        void collectionChanged();
        void collectionAboutToDelete();
        void collectionLockChanged(bool newLocked);

        void aliasAboutToAdd(const QString& alias);
        void aliasAdded(const QString& alias);
        void aliasRemoved(const QString& alias);

        void doneUnlockCollection(bool accepted);

    public:
        DBusReturn<void> setProperties(const QVariantMap& properties);

        bool isValid() const
        {
            return backend();
        }

        DBusReturn<void> removeAlias(QString alias);
        DBusReturn<void> addAlias(QString alias);
        const QSet<QString> aliases() const;

        /**
         * A human readable name of the collection, available even if the db is locked
         * @return
         */
        QString name() const;

        Group* exposedRootGroup() const;
        DatabaseWidget* backend() const;
        QString backendFilePath() const;
        Service* service() const;
        bool inRecycleBin(Group* group) const;
        bool inRecycleBin(Entry* entry) const;

        static EntrySearcher::SearchTerm attributeToTerm(const QString& key, const QString& value);

    public slots:
        // expose some methods for Prompt to use
        bool doLock();
        void doUnlock();
        // will remove self
        void doDelete();

        // delete the Entry in backend from this collection
        void doDeleteEntries(QList<Entry*> entries);

        // force reload info from backend, potentially delete self
        bool reloadBackend();

    private slots:
        void onDatabaseLockChanged();
        void onDatabaseExposedGroupChanged();

        // calls reloadBackend, delete self when error
        void reloadBackendOrDelete();

    private:
        friend class DeleteCollectionPrompt;
        friend class CreateCollectionPrompt;

        void onEntryAdded(Entry* entry, bool emitSignal);
        void populateContents();
        void connectGroupSignalRecursive(Group* group);
        void cleanupConnections();

        bool backendLocked() const;

        /**
         * Check if the backend is a valid object, send error reply if not.
         * @return true if the backend is valid.
         */
        DBusReturn<void> ensureBackend() const;

        /**
         * Ensure the database is unlocked, send error reply if locked.
         * @return true if the database is locked
         */
        DBusReturn<void> ensureUnlocked() const;

        /**
         * Like mkdir -p, find or create the group by path, under m_exposedGroup
         * @param groupPath
         * @return created or found group
         */
        Group* findCreateGroupByPath(const QString& groupPath);

    private:
        QPointer<DatabaseWidget> m_backend;
        QString m_backendPath;
        QPointer<Group> m_exposedGroup;

        QSet<QString> m_aliases;
        QList<Item*> m_items;
        QMap<const Entry*, Item*> m_entryToItem;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_COLLECTION_H
