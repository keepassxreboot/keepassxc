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

#include "ServiceAdaptor.h"

#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

namespace FdoSecrets
{

    ServiceAdaptor::ServiceAdaptor(Service* parent)
        : DBusAdaptor(parent)
    {
        // p() isn't ready yet as this is called in Parent's constructor
        connect(parent, &Service::collectionCreated, this, [this](Collection* coll) {
            emit CollectionCreated(objectPathSafe(coll));
        });
        connect(parent, &Service::collectionDeleted, this, [this](Collection* coll) {
            emit CollectionDeleted(objectPathSafe(coll));
        });
        connect(parent, &Service::collectionChanged, this, [this](Collection* coll) {
            emit CollectionChanged(objectPathSafe(coll));
        });
    }

    const QList<QDBusObjectPath> ServiceAdaptor::collections() const
    {
        auto colls = p()->collections().valueOrHandle(p());
        return objectsToPath(std::move(colls));
    }

    QDBusVariant
    ServiceAdaptor::OpenSession(const QString& algorithm, const QDBusVariant& input, QDBusObjectPath& result)
    {
        Session* session = nullptr;
        auto output = p()->openSession(algorithm, input.variant(), session).valueOrHandle(p());
        result = objectPathSafe(session);
        return QDBusVariant(std::move(output));
    }

    QDBusObjectPath
    ServiceAdaptor::CreateCollection(const QVariantMap& properties, const QString& alias, QDBusObjectPath& prompt)
    {
        PromptBase* pp;
        auto coll = p()->createCollection(properties, alias, pp).valueOrHandle(p());
        prompt = objectPathSafe(pp);
        return objectPathSafe(coll);
    }

    const QList<QDBusObjectPath> ServiceAdaptor::SearchItems(const StringStringMap& attributes,
                                                             QList<QDBusObjectPath>& locked)
    {
        QList<Item*> lockedItems, unlockedItems;
        unlockedItems = p()->searchItems(attributes, lockedItems).valueOrHandle(p());
        locked = objectsToPath(lockedItems);
        return objectsToPath(unlockedItems);
    }

    const QList<QDBusObjectPath> ServiceAdaptor::Unlock(const QList<QDBusObjectPath>& paths, QDBusObjectPath& prompt)
    {
        auto objects = pathsToObject<DBusObject>(paths);
        if (!paths.isEmpty() && objects.isEmpty()) {
            DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT)).handle(p());
            return {};
        }

        PromptBase* pp = nullptr;
        auto unlocked = p()->unlock(objects, pp).valueOrHandle(p());

        prompt = objectPathSafe(pp);
        return objectsToPath(unlocked);
    }

    const QList<QDBusObjectPath> ServiceAdaptor::Lock(const QList<QDBusObjectPath>& paths, QDBusObjectPath& prompt)
    {
        auto objects = pathsToObject<DBusObject>(paths);
        if (!paths.isEmpty() && objects.isEmpty()) {
            DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT)).handle(p());
            return {};
        }

        PromptBase* pp = nullptr;
        auto locked = p()->lock(objects, pp).valueOrHandle(p());

        prompt = objectPathSafe(pp);
        return objectsToPath(locked);
    }

    const ObjectPathSecretMap ServiceAdaptor::GetSecrets(const QList<QDBusObjectPath>& items,
                                                         const QDBusObjectPath& session)
    {
        auto itemObjects = pathsToObject<Item>(items);
        if (!items.isEmpty() && itemObjects.isEmpty()) {
            DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT)).handle(p());
            return {};
        }

        auto secrets = p()->getSecrets(pathsToObject<Item>(items), pathToObject<Session>(session)).valueOrHandle(p());

        ObjectPathSecretMap res;
        auto iter = secrets.begin();
        while (iter != secrets.end()) {
            res[objectPathSafe(iter.key())] = std::move(iter.value());
            ++iter;
        }
        return res;
    }

    QDBusObjectPath ServiceAdaptor::ReadAlias(const QString& name)
    {
        auto coll = p()->readAlias(name).valueOrHandle(p());
        return objectPathSafe(coll);
    }

    void ServiceAdaptor::SetAlias(const QString& name, const QDBusObjectPath& collection)
    {
        p()->setAlias(name, pathToObject<Collection>(collection)).handle(p());
    }

} // namespace FdoSecrets
