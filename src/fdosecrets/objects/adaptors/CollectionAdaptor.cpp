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

#include "CollectionAdaptor.h"

#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"

namespace FdoSecrets
{

    CollectionAdaptor::CollectionAdaptor(Collection* parent)
        : DBusAdaptor(parent)
    {
        // p() isn't ready yet as this is called in Parent's constructor
        connect(parent, &Collection::itemCreated, this, [this](const Item* item) {
            emit ItemCreated(objectPathSafe(item));
        });
        connect(parent, &Collection::itemDeleted, this, [this](const Item* item) {
            emit ItemDeleted(objectPathSafe(item));
        });
        connect(parent, &Collection::itemChanged, this, [this](const Item* item) {
            emit ItemChanged(objectPathSafe(item));
        });
    }

    const QList<QDBusObjectPath> CollectionAdaptor::items() const
    {
        return objectsToPath(p()->items().valueOrHandle(p()));
    }

    QString CollectionAdaptor::label() const
    {
        return p()->label().valueOrHandle(p());
    }

    void CollectionAdaptor::setLabel(const QString& label)
    {
        p()->setLabel(label).handle(p());
    }

    bool CollectionAdaptor::locked() const
    {
        return p()->locked().valueOrHandle(p());
    }

    qulonglong CollectionAdaptor::created() const
    {
        return p()->created().valueOrHandle(p());
    }

    qulonglong CollectionAdaptor::modified() const
    {
        return p()->modified().valueOrHandle(p());
    }

    QDBusObjectPath CollectionAdaptor::Delete()
    {
        return objectPathSafe(p()->deleteCollection().valueOrHandle(p()));
    }

    QList<QDBusObjectPath> CollectionAdaptor::SearchItems(const StringStringMap& attributes)
    {
        return objectsToPath(p()->searchItems(attributes).valueOrHandle(p()));
    }

    QDBusObjectPath CollectionAdaptor::CreateItem(const QVariantMap& properties,
                                                  const SecretStruct& secret,
                                                  bool replace,
                                                  QDBusObjectPath& prompt)
    {
        PromptBase* pp = nullptr;
        auto item = p()->createItem(properties, secret, replace, pp).valueOrHandle(p());
        prompt = objectPathSafe(pp);
        return objectPathSafe(item);
    }

} // namespace FdoSecrets
