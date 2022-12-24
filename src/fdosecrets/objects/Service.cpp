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

namespace
{
    constexpr auto DEFAULT_ALIAS = "default";
}

namespace FdoSecrets
{
    QSharedPointer<Service> Service::Create(FdoSecretsPlugin* plugin, QSharedPointer<DBusMgr> dbus)
    {
        QSharedPointer<Service> res{new Service(plugin, std::move(dbus))};
        if (!res->dbus()->registerObject(res.data())) {
            return nullptr;
        }

        return res;
    }

    Service::Service(FdoSecretsPlugin* plugin,
                     QSharedPointer<DBusMgr> dbus) // clazy: exclude=ctor-missing-parent-argument
        : DBusObject(std::move(dbus))
        , m_plugin(plugin)
    {
    }

    Service::~Service() = default;

    void Service::ensureDefaultAlias()
    {
        auto coll = m_aliasToCollection.value(DEFAULT_ALIAS);
        if (!m_collections.isEmpty() && !coll) {
            m_aliasToCollection[DEFAULT_ALIAS] = m_collections.first();
        }
    }

    /**
     * D-Bus Properties
     */

    DBusResult Service::collections(QList<Collection*>& collections) const
    {
        collections = m_collections;
        return {};
    }

    DBusResult Service::unlockedCollections(QList<Collection*>& unlocked) const
    {
        auto ret = collections(unlocked);
        if (ret.err()) {
            return ret;
        }

        // filter out locked collections
        auto it = unlocked.begin();
        while (it != unlocked.end()) {
            bool isLocked = true;
            ret = (*it)->locked(isLocked);
            if (ret.err()) {
                return ret;
            }
            if (isLocked) {
                it = unlocked.erase(it);
            } else {
                ++it;
            }
        }
        return {};
    }

    /**
     * D-Bus Methods
     */

    DBusResult Service::openSession(const DBusClientPtr& client,
                                    const QString& algorithm,
                                    const QVariant& input,
                                    QVariant& output,
                                    Session*& result)
    {
        // negotiate cipher
        bool incomplete = false;
        auto ciphers = client->negotiateCipher(algorithm, input, output, incomplete);
        if (incomplete) {
            result = nullptr;
            return {};
        }
        if (!ciphers) {
            return QDBusError::NotSupported;
        }

        // create session using the negotiated cipher
        result = Session::Create(std::move(ciphers), client->name(), this);
        if (!result) {
            return QDBusError::InternalError;
        }

        // remove session when the client disconnects
        connect(dbus().data(), &DBusMgr::clientDisconnected, result, [result, client](const DBusClientPtr& toRemove) {
            if (toRemove == client) {
                result->close().okOrDie();
            }
        });

        // keep a list of sessions
        m_sessions.append(result);
        connect(result, &Session::aboutToClose, this, [this, result]() { m_sessions.removeAll(result); });

        return {};
    }

    DBusResult Service::createCollection(const QVariantMap& properties,
                                         const QString& alias,
                                         Collection*& collection,
                                         PromptBase*& prompt)
    {
        prompt = nullptr;

        // return existing collection if alias is non-empty and exists.
        collection = m_aliasToCollection.value(alias);
        if (!collection) {
            prompt = PromptBase::Create<CreateCollectionPrompt>(this, properties, alias);
            if (!prompt) {
                return QDBusError::InternalError;
            }
        }
        return {};
    }

    DBusResult Service::searchItems(const DBusClientPtr& client,
                                    const StringStringMap& attributes,
                                    QList<Item*>& unlocked,
                                    QList<Item*>& locked)
    {
        // we can only search unlocked collections
        QList<Collection*> unlockedColls;
        auto ret = unlockedCollections(unlockedColls);
        if (ret.err()) {
            return ret;
        }

        while (unlockedColls.isEmpty() && settings()->unlockBeforeSearch()) {
            // enable compatibility mode by making sure at least one database is unlocked
            bool wasAccepted = plugin()->requestUnlockAnyDatabase(client);

            if (!wasAccepted) {
                // user cancelled, do not proceed
                qWarning() << "user cancelled";
                return {};
            }

            // need to recompute this because collections may disappear while in event loop
            ret = unlockedCollections(unlockedColls);
            if (ret.err()) {
                return ret;
            }
        }

        for (const auto& coll : asConst(unlockedColls)) {
            QList<Item*> items;
            ret = coll->searchItems(client, attributes, items);
            if (ret.err()) {
                return ret;
            }
            // item locked state already covers its collection's locked state
            for (const auto& item : asConst(items)) {
                Q_ASSERT(item);
                bool itemLocked;
                ret = item->locked(client, itemLocked);
                if (ret.err()) {
                    return ret;
                }
                if (itemLocked) {
                    locked.append(item);
                } else {
                    unlocked.append(item);
                }
            }
        }
        return {};
    }

    DBusResult Service::unlock(const DBusClientPtr& client,
                               const QList<DBusObject*>& objects,
                               QList<DBusObject*>& unlocked,
                               PromptBase*& prompt)
    {
        QSet<Collection*> collectionsToUnlock;
        QSet<Item*> itemsToUnlock;
        collectionsToUnlock.reserve(objects.size());
        itemsToUnlock.reserve(objects.size());

        for (const auto& obj : asConst(objects)) {
            // the object is either an item or a collection
            auto item = qobject_cast<Item*>(obj);
            auto coll = item ? item->collection() : qobject_cast<Collection*>(obj);
            // either way there should be a collection
            if (!coll) {
                continue;
            }

            bool collLocked{false}, itemLocked{false};
            // if the collection needs unlock
            auto ret = coll->locked(collLocked);
            if (ret.err()) {
                return ret;
            }
            if (collLocked) {
                collectionsToUnlock << coll;
            }

            if (item) {
                // item may also need unlock
                ret = item->locked(client, itemLocked);
                if (ret.err()) {
                    return ret;
                }
                if (itemLocked) {
                    itemsToUnlock << item;
                }
            }

            // both collection and item are not locked
            if (!collLocked && !itemLocked) {
                unlocked << obj;
            }
        }

        prompt = nullptr;
        if (!collectionsToUnlock.isEmpty() || !itemsToUnlock.isEmpty()) {
            prompt = PromptBase::Create<UnlockPrompt>(this, collectionsToUnlock, itemsToUnlock);
            if (!prompt) {
                return QDBusError::InternalError;
            }
        }
        return {};
    }

    DBusResult Service::lock(const QList<DBusObject*>& objects, QList<DBusObject*>& locked, PromptBase*& prompt)
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
        QList<Collection*> toLock;
        for (const auto& coll : asConst(needLock)) {
            bool l;
            auto ret = coll->locked(l);
            if (ret.err()) {
                return ret;
            }
            if (l) {
                locked << coll;
            } else {
                toLock << coll;
            }
        }
        if (!toLock.isEmpty()) {
            prompt = PromptBase::Create<LockCollectionsPrompt>(this, toLock);
            if (!prompt) {
                return QDBusError::InternalError;
            }
        }
        return {};
    }

    DBusResult Service::getSecrets(const DBusClientPtr& client,
                                   const QList<Item*>& items,
                                   Session* session,
                                   ItemSecretMap& secrets) const
    {
        if (!session) {
            return DBusResult(DBUS_ERROR_SECRET_NO_SESSION);
        }

        for (const auto& item : asConst(items)) {
            auto ret = item->getSecretNoNotification(client, session, secrets[item]);
            if (ret.err()) {
                return ret;
            }
        }
        plugin()->emitRequestShowNotification(
            tr(R"(%n Entry(s) was used by %1)", "%1 is the name of an application", secrets.size())
                .arg(client->name()));
        return {};
    }

    DBusResult Service::readAlias(const QString& name, Collection*& collection) const
    {
        collection = m_aliasToCollection.value(name);
        return {};
    }

    DBusResult Service::setAlias(const QString& name, Collection* collection)
    {
        if (!collection) {
            if (!m_aliasToCollection.remove(name)) {
                return DBusResult(DBUS_ERROR_SECRET_NO_SUCH_OBJECT);
            }
            ensureDefaultAlias();
            return {};
        }

        m_aliasToCollection[name] = collection;
        return {};
    }

    QList<Session*> Service::sessions() const
    {
        return m_sessions;
    }

    Collection* Service::doNewDatabase(const DBusClientPtr& client)
    {
        auto name = plugin()->requestNewDatabase(client);
        return m_nameToCollection.value(name);
    }

    bool Service::setAlias(const QString& alias, const QString& name)
    {
        Q_ASSERT(!name.isEmpty());
        return !setAlias(alias, m_nameToCollection.value(name)).err();
    }

    QStringList Service::collectionAliases(const Collection* collection) const
    {
        QStringList result;
        for (auto it = m_aliasToCollection.constBegin(); it != m_aliasToCollection.constEnd(); ++it) {
            if (it.value() == collection) {
                result << it.key();
            }
        }

        return result;
    }

    Collection* Service::createCollection(const QString& name)
    {
        Q_ASSERT(!name.isEmpty());
        auto coll = m_nameToCollection.value(name);
        if (coll) {
            return coll;
        }

        coll = Collection::Create(this, name);
        if (!coll) {
            return nullptr;
        }

        m_collections << coll;
        m_nameToCollection[name] = coll;

        // keep record of the collection existence
        connect(coll, &Collection::collectionChanged, this, [this, coll]() { emit collectionChanged(coll); });
        connect(coll, &Collection::collectionAboutToDelete, this, [this, coll]() {
            m_collections.removeAll(coll);
            m_nameToCollection.remove(coll->name());
            emit collectionDeleted(coll);

            for (auto it = m_aliasToCollection.begin(); it != m_aliasToCollection.end();) {
                if (*it == coll) {
                    it = m_aliasToCollection.erase(it);
                } else {
                    ++it;
                }
            }
        });

        ensureDefaultAlias();
        emit collectionCreated(coll);
        return coll;
    }

    void Service::registerDatabase(const QString& name)
    {
        Q_ASSERT(!name.isEmpty());
        if (!m_nameToCollection.value(name)) {
            createCollection(name);
        }
    }

    void Service::unregisterDatabase(const QString& name)
    {
        auto coll = m_nameToCollection.value(name);
        if (coll) {
            coll->removeFromDBus();
        }
    }

    void Service::databaseLocked(const QString& name)
    {
        auto coll = m_nameToCollection.value(name);
        if (coll) {
            coll->databaseLocked();
        }
    }

    void Service::databaseUnlocked(const QString& name, QSharedPointer<Database> db)
    {
        Q_ASSERT(!name.isEmpty());
        auto coll = m_nameToCollection.value(name);
        if (!coll) {
            coll = createCollection(name);
        }

        coll->databaseUnlocked(db);
    }

} // namespace FdoSecrets
