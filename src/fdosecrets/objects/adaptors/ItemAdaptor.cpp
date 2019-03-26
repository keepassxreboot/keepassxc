/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "ItemAdaptor.h"

#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Session.h"

namespace FdoSecrets
{

    ItemAdaptor::ItemAdaptor(Item* parent)
        : DBusAdaptor(parent)
    {
    }

    bool ItemAdaptor::locked() const
    {
        return p()->locked().valueOrHandle(p());
    }

    const StringStringMap ItemAdaptor::attributes() const
    {
        return p()->attributes().valueOrHandle(p());
    }

    void ItemAdaptor::setAttributes(const StringStringMap& attrs)
    {
        p()->setAttributes(attrs).handle(p());
    }

    QString ItemAdaptor::label() const
    {
        return p()->label().valueOrHandle(p());
    }

    void ItemAdaptor::setLabel(const QString& label)
    {
        p()->setLabel(label).handle(p());
    }

    qulonglong ItemAdaptor::created() const
    {
        return p()->created().valueOrHandle(p());
    }

    qulonglong ItemAdaptor::modified() const
    {
        return p()->modified().valueOrHandle(p());
    }

    QDBusObjectPath ItemAdaptor::Delete()
    {
        auto prompt = p()->deleteItem().valueOrHandle(p());
        return objectPathSafe(prompt);
    }

    SecretStruct ItemAdaptor::GetSecret(const QDBusObjectPath& session)
    {
        return p()->getSecret(pathToObject<Session>(session)).valueOrHandle(p());
    }

    void ItemAdaptor::SetSecret(const SecretStruct& secret)
    {
        p()->setSecret(secret).handle(p());
    }

} // namespace FdoSecrets
