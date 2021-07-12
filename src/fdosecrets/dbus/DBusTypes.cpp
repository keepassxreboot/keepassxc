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

#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include <QDBusMetaType>

namespace FdoSecrets
{
    bool inherits(const QMetaObject* derived, const QMetaObject* base)
    {
        for (auto super = derived; super; super = super->superClass()) {
            if (super == base) {
                return true;
            }
        }
        return false;
    }

    template <typename T> void registerConverter(const QWeakPointer<DBusMgr>& weak)
    {
        // from parameter type to on-the-wire type
        QMetaType::registerConverter<T*, QDBusObjectPath>([](const T* obj) { return DBusMgr::objectPathSafe(obj); });
        QMetaType::registerConverter<QList<T*>, QList<QDBusObjectPath>>(
            [](const QList<T*> objs) { return DBusMgr::objectsToPath(objs); });

        // the opposite
        QMetaType::registerConverter<QDBusObjectPath, T*>([weak](const QDBusObjectPath& path) -> T* {
            if (auto dbus = weak.lock()) {
                return dbus->pathToObject<T>(path);
            }
            qDebug() << "No DBusMgr when looking up path" << path.path();
            return nullptr;
        });
        QMetaType::registerConverter<QList<QDBusObjectPath>, QList<T*>>([weak](const QList<QDBusObjectPath>& paths) {
            if (auto dbus = weak.lock()) {
                return dbus->pathsToObject<T>(paths);
            }
            qDebug() << "No DBusMgr when looking up paths";
            return QList<T*>{};
        });
    }

    void registerDBusTypes(const QSharedPointer<DBusMgr>& dbus)
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

        // NOTE: when registering, in additional to the class' fully qualified name,
        // the partial-namespace/non-namespace name should also be registered as alias
        // otherwise all those usages in Q_INVOKABLE methods without FQN won't be included
        // in the meta type system.
#define REG_METATYPE(type)                                                                                             \
    qRegisterMetaType<type>();                                                                                         \
    qRegisterMetaType<type>(#type)

        // register on-the-wire types
        // Qt container types for builtin types don't need registration
        REG_METATYPE(wire::Secret);
        REG_METATYPE(wire::StringStringMap);
        REG_METATYPE(wire::ObjectPathSecretMap);

        qDBusRegisterMetaType<wire::Secret>();
        qDBusRegisterMetaType<wire::StringStringMap>();
        qDBusRegisterMetaType<wire::ObjectPathSecretMap>();

        // register parameter types
        REG_METATYPE(Secret);
        REG_METATYPE(StringStringMap);
        REG_METATYPE(ItemSecretMap);
        REG_METATYPE(DBusResult);
        REG_METATYPE(DBusClientPtr);

#define REG_DBUS_OBJ(name)                                                                                             \
    REG_METATYPE(name*);                                                                                               \
    REG_METATYPE(QList<name*>)
        REG_DBUS_OBJ(DBusObject);
        REG_DBUS_OBJ(Service);
        REG_DBUS_OBJ(Collection);
        REG_DBUS_OBJ(Item);
        REG_DBUS_OBJ(Session);
        REG_DBUS_OBJ(PromptBase);
#undef REG_DBUS_OBJ

#undef REG_METATYPE

        QWeakPointer<DBusMgr> weak = dbus;
        // register converter between on-the-wire types and parameter types
        // some pairs are missing because that particular direction isn't used
        registerConverter<DBusObject>(weak);
        registerConverter<Service>(weak);
        registerConverter<Collection>(weak);
        registerConverter<Item>(weak);
        registerConverter<Session>(weak);
        registerConverter<PromptBase>(weak);

        QMetaType::registerConverter<wire::Secret, Secret>(
            [weak](const wire::Secret& from) { return from.unmarshal(weak); });
        QMetaType::registerConverter(&Secret::marshal);

        QMetaType::registerConverter<ItemSecretMap, wire::ObjectPathSecretMap>([](const ItemSecretMap& map) {
            wire::ObjectPathSecretMap ret;
            for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                ret.insert(it.key()->objectPath(), it.value().marshal());
            }
            return ret;
        });

        QMetaType::registerConverter<QDBusVariant, QVariant>([](const QDBusVariant& obj) { return obj.variant(); });
        QMetaType::registerConverter<QVariant, QDBusVariant>([](const QVariant& obj) { return QDBusVariant(obj); });

        // structural types are received as QDBusArgument,
        // top level QDBusArgument in method parameters are directly handled
        // in prepareInputParams.
        // But in Collection::createItem, we need to convert a inner QDBusArgument to StringStringMap
        QMetaType::registerConverter<QDBusArgument, StringStringMap>([](const QDBusArgument& arg) {
            if (arg.currentSignature() != "a{ss}") {
                return StringStringMap{};
            }
            // QDBusArgument is COW and qdbus_cast modifies it by detaching even it is const.
            // we don't want to modify the instance (arg) stored in the qvariant so we create a copy
            const auto copy = arg; // NOLINT(performance-unnecessary-copy-initialization)
            return qdbus_cast<StringStringMap>(copy);
        });
    }

    ParamData typeToWireType(int id)
    {
        switch (id) {
        case QMetaType::QString:
            return {QByteArrayLiteral("s"), QMetaType::QString};
        case QMetaType::QVariant:
            return {QByteArrayLiteral("v"), qMetaTypeId<QDBusVariant>()};
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
        if (QByteArray(QMetaType::typeName(id)).startsWith("QList")) {
            // QList<Object*>
            return {QByteArrayLiteral("ao"), qMetaTypeId<QList<QDBusObjectPath>>()};
        }
        if (!inherits(mt.metaObject(), &DBusObject::staticMetaObject)) {
            return {};
        }
        // DBusObjects
        return {QByteArrayLiteral("o"), qMetaTypeId<QDBusObjectPath>()};
    }

    ::FdoSecrets::Secret wire::Secret::unmarshal(const QWeakPointer<DBusMgr>& weak) const
    {
        if (auto dbus = weak.lock()) {
            return {dbus->pathToObject<Session>(session), parameters, value, contentType};
        }
        qDebug() << "No DBusMgr when converting wire::Secret";
        return {nullptr, parameters, value, contentType};
    }

    wire::Secret Secret::marshal() const
    {
        return {DBusMgr::objectPathSafe(session), parameters, value, contentType};
    }

} // namespace FdoSecrets
