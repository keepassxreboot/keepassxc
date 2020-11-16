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

#ifndef KEEPASSXC_FDOSECRETS_DBUSOBJECT_H
#define KEEPASSXC_FDOSECRETS_DBUSOBJECT_H

#include "fdosecrets/objects/DBusReturn.h"
#include "fdosecrets/objects/DBusTypes.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QDebug>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QScopedPointer>

namespace FdoSecrets
{
    class Service;

    /**
     * @brief A common base class for all dbus-exposed objects.
     * However, derived class should inherit from `DBusObjectHelper`, which is
     * the only way to set DBus adaptor and enforces correct adaptor creation.
     */
    class DBusObject : public QObject, public QDBusContext
    {
        Q_OBJECT
    public:
        const QDBusObjectPath& objectPath() const
        {
            return m_objectPath;
        }

        QDBusAbstractAdaptor& dbusAdaptor() const
        {
            return *m_dbusAdaptor;
        }

    protected:
        /**
         * @brief Register this object at given DBus path
         * @param path DBus path to register at
         * @param primary whether this path to be considered primary. The primary path is the one to be returned by
         * `DBusObject::objectPath`.
         * @return true on success
         */
        bool registerWithPath(const QString& path, bool primary = true);

        void unregisterPrimaryPath()
        {
            if (m_objectPath.path() == QStringLiteral("/")) {
                return;
            }
            QDBusConnection::sessionBus().unregisterObject(m_objectPath.path());
            m_objectPath.setPath(QStringLiteral("/"));
        }

        QString callingPeer() const
        {
            Q_ASSERT(calledFromDBus());
            return message().service();
        }

        uint callingPeerPid() const
        {
            return connection().interface()->servicePid(callingPeer());
        }

        QString callingPeerName() const;

        DBusObject* p() const
        {
            return qobject_cast<DBusObject*>(parent());
        }

    private:
        explicit DBusObject(DBusObject* parent);

        /**
         * Derived class should not directly use sendErrorReply.
         * Instead, use raiseError
         */
        using QDBusContext::sendErrorReply;

        template <typename U> friend class DBusReturn;
        template <typename Object, typename Adaptor> friend class DBusObjectHelper;

        QDBusAbstractAdaptor* m_dbusAdaptor;
        QDBusObjectPath m_objectPath;
    };

    template <typename Object, typename Adaptor> class DBusObjectHelper : public DBusObject
    {
    protected:
        explicit DBusObjectHelper(DBusObject* parent)
            : DBusObject(parent)
        {
            // creating new Adaptor has to be delayed into constructor's body,
            // and can't be simply moved to initializer list, because at that
            // point the base QObject class hasn't been initialized and will sigfault.
            m_dbusAdaptor = new Adaptor(static_cast<Object*>(this));
            m_dbusAdaptor->setParent(this);
        }
    };

    /**
     * Return the object path of the pointed DBusObject, or "/" if the pointer is null
     * @tparam T
     * @param object
     * @return
     */
    template <typename T> QDBusObjectPath objectPathSafe(T* object)
    {
        if (object) {
            return object->objectPath();
        }
        return QDBusObjectPath(QStringLiteral("/"));
    }

    /**
     * Convert a list of DBusObjects to object path
     * @tparam T
     * @param objects
     * @return
     */
    template <typename T> QList<QDBusObjectPath> objectsToPath(QList<T*> objects)
    {
        QList<QDBusObjectPath> res;
        res.reserve(objects.size());
        for (auto object : objects) {
            res.append(objectPathSafe(object));
        }
        return res;
    }

    /**
     * Convert an object path to a pointer of the object
     * @tparam T
     * @param path
     * @return the pointer of the object, or nullptr if path is "/"
     */
    template <typename T> T* pathToObject(const QDBusObjectPath& path)
    {
        if (path.path() == QStringLiteral("/")) {
            return nullptr;
        }
        return qobject_cast<T*>(QDBusConnection::sessionBus().objectRegisteredAt(path.path()));
    }

    /**
     * Convert a list of object paths to a list of objects.
     * "/" paths (i.e. nullptrs) will be skipped in the resulting list
     * @tparam T
     * @param paths
     * @return
     */
    template <typename T> QList<T*> pathsToObject(const QList<QDBusObjectPath>& paths)
    {
        QList<T*> res;
        res.reserve(paths.size());
        for (const auto& path : paths) {
            auto object = pathToObject<T>(path);
            if (object) {
                res.append(object);
            }
        }
        return res;
    }

    /**
     * Encode the string value to a DBus object path safe representation,
     * using a schema similar to URI encoding, but with percentage(%) replaced with
     * underscore(_). All characters except [A-Za-z0-9] are encoded. For non-ascii
     * characters, UTF-8 encoding is first applied and each of the resulting byte
     * value is encoded.
     * @param value
     * @return encoded string
     */
    QString encodePath(const QString& value);

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSOBJECT_H
