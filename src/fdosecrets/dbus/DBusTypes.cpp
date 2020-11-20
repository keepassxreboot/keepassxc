/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
 *  Copyright 2010, Michael Leupold <lemma@confuego.org>
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

#include "DBusTypes.h"

#include <QDBusMetaType>

#include "DBusMgr.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

namespace FdoSecrets
{
    template <typename T> void registerConverter()
    {
        qRegisterMetaType<T*>();
        qRegisterMetaType<QList<T*>>();

        // from parameter type to on-the-wire type
        QMetaType::registerConverter<T*, QDBusObjectPath>([](const T* obj) { return DBusMgr::objectPathSafe(obj); });
        QMetaType::registerConverter<QList<T*>, QList<QDBusObjectPath>>(
            [](const QList<T*> objs) { return DBusMgr::objectsToPath(objs); });

        // the opposite
        QMetaType::registerConverter<QDBusObjectPath, T*>([](const QDBusObjectPath& path) -> T* {
            return DBusMgr::pathToObject<T>(path);
        });
        QMetaType::registerConverter<QList<QDBusObjectPath>, QList<T*>>([](const QList<QDBusObjectPath>& paths) {
            return DBusMgr::pathsToObject<T>(paths);
        });
    }

    void registerDBusTypes()
    {
        // On the wire types:
        // - various primary types
        // - QDBusVariant
        // - wire::Secret
        // - wire::ObjectPathSecretMap
        // - QDBusObjectPath
        // - QList<QDBusObjectPath>

        // Parameter types:
        // - various primary types
        // - QVariant
        // - Secret
        // - ObjectSecretMap
        // - DBusObject* (and derived classes)
        // - QList<DBusObject*>

        // register on-the-wire types
        // Qt container types for builtin types don't need registration
        qRegisterMetaType<wire::Secret>();
        qRegisterMetaType<wire::StringStringMap>();
        qRegisterMetaType<wire::ObjectPathSecretMap>();

        qDBusRegisterMetaType<wire::Secret>();
        qDBusRegisterMetaType<wire::StringStringMap>();
        qDBusRegisterMetaType<wire::ObjectPathSecretMap>();

        // register parameter types
        qRegisterMetaType<Secret>();
        qRegisterMetaType<StringStringMap>();
        qRegisterMetaType<ItemSecretMap>();
        // DBusObject* types are registered in the converter below
        qRegisterMetaType<DBusResult>();

        // register converter between on-the-wire types and parameter types
        // some pairs are missing because that particular direction isn't used
        registerConverter<DBusObject>();
        registerConverter<Service>();
        registerConverter<Collection>();
        registerConverter<Item>();
        registerConverter<Session>();
        registerConverter<PromptBase>();
        QMetaType::registerConverter(&wire::Secret::to);
        QMetaType::registerConverter(&Secret::to);

        QMetaType::registerConverter<ItemSecretMap, wire::ObjectPathSecretMap>(
            [](const ItemSecretMap& map) {
                wire::ObjectPathSecretMap ret;
                auto it = map.constKeyValueBegin();
                auto end = map.constKeyValueEnd();
                for (; it != end; ++it) {
                    ret.insert(it->first->objectPath(), it->second.to());
                }
                return ret;
            });

        QMetaType::registerConverter<QDBusVariant, QVariant>([](const QDBusVariant& obj) { return obj.variant(); });
        QMetaType::registerConverter<QVariant, QDBusVariant>([](const QVariant& obj) { return QDBusVariant(obj); });
    }

    ParamData typeToWireType(int id)
    {
        switch (id)
        {
        case QMetaType::QString:
            return {QByteArrayLiteral("s"), QMetaType::QString};
        case QMetaType::QVariant:
            return {QByteArrayLiteral("v"), QMetaType::QVariant};
        case QMetaType::QVariantMap:
            return {QByteArrayLiteral("a{sv}"), QMetaType::QVariantMap};
        case QMetaType::Bool:
            return {QByteArrayLiteral("b"), QMetaType::Bool};
        case QMetaType::ULongLong:
            return {QByteArrayLiteral("t"), QMetaType::ULongLong};
        default:
            break;
        }
        if (id == qMetaTypeId<StringStringMap>()) {
            return {QByteArrayLiteral("a{ss}"), qMetaTypeId<wire::StringStringMap>()};
        } else if (id == qMetaTypeId<ItemSecretMap>()) {
            return {QByteArrayLiteral("a{o(oayays)}"), qMetaTypeId<wire::ObjectPathSecretMap>()};
        } else if (id == qMetaTypeId<Secret>()) {
            return {QByteArrayLiteral("(oayays)"), qMetaTypeId<wire::Secret>()};
        } else if (id == qMetaTypeId<DBusObject*>()) {
            return {QByteArrayLiteral("o"), qMetaTypeId<QDBusObjectPath>()};
        } else if (id == qMetaTypeId<QList<DBusObject*>>()) {
            return {QByteArrayLiteral("ao"), qMetaTypeId<QList<QDBusObjectPath>>()};
        }

        QMetaType mt(id);
        if (!mt.isValid()) {
            return {};
        }
        if (mt.name().startsWith("QList")) {
            // QList<Object*>
            return {QByteArrayLiteral("ao"), qMetaTypeId<QList<QDBusObjectPath>>()};
        }
        if (!mt.metaObject()->inherits(&DBusObject::staticMetaObject)) {
            return {};
        }
        // DBusObjects
        return {QByteArrayLiteral("o"), qMetaTypeId<QDBusObjectPath>()};
    }

    ::FdoSecrets::Secret wire::Secret::to() const
    {
        return {DBusMgr::pathToObject<Session>(session), parameters, value, contentType};
    }

    wire::Secret Secret::to() const
    {
        return {DBusMgr::objectPathSafe(session), parameters, value, contentType};
    }

} // namespace FdoSecrets
