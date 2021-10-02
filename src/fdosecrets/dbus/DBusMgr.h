/*
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcode.works>
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

#ifndef KEEPASSXC_FDOSECRETS_DBUSMGR_H
#define KEEPASSXC_FDOSECRETS_DBUSMGR_H

#include "fdosecrets/dbus/DBusClient.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusVirtualObject>
#include <QDebug>
#include <QtDBus>

class TestFdoSecrets;

namespace FdoSecrets
{
    class Collection;
    class Service;
    class PromptBase;
    class Session;
    class Item;
    class DBusObject;
    class DBusResult;

    /**
     * DBusMgr takes care of the interaction between dbus and business logic objects (DBusObject). It handles the
     * following
     * - Registering/unregistering service name
     * - Registering/unregistering paths
     * - Relay signals from DBusObject to dbus
     * - Manage per-client states, mapping from dbus caller address to Client
     * - Deliver method calls from dbus to DBusObject
     *
     * Special note in implementation of method delivery:
     * There are two sets of vocabulary classes in use for method delivery.
     * The Qt DBus system uses QDBusVariant/QDBusObjectPath and other primitive types in QDBusMessage::arguments(),
     * i.e. the on-the-wire types.
     * The DBusObject invokable methods uses QVariant/DBusObject* and other primitive types in parameters (parameter
     * types). FdoSecrets::typeToWireType establishes the mapping from parameter types to on-the-wire types. The
     * conversion between types is done with the help of QMetaType convert.
     *
     * The method delivery sequence:
     * - DBusMgr::handleMessage unifies method call and property access into the same form
     * - DBusMgr::activateObject finds the target object and calls the method by doing the following
     *   * check the object exists and the interface matches
     *   * find the cached method information MethodData
     *   * DBusMgr::prepareInputParams check and convert input arguments in QDBusMessage::arguments() to types expected
     * by DBusObject
     *   * prepare output argument storage
     *   * call the method
     *   * convert types to what Qt DBus expects
     *
     * The MethodData is pre-computed using Qt meta object system by finding methods with signature matching a certain
     * pattern:
     * Q_INVOKABLE DBusResult methodName(const DBusClientPtr& client,
     *                                   const X& input1,
     *                                   const Y& input2,
     *                                   Z& output1,
     *                                   ZZ& output2)
     * Note that the first parameter of client is optional.
     */
    class DBusMgr : public QDBusVirtualObject
    {
        Q_OBJECT
    public:
        explicit DBusMgr();

        /**
         * @brief Must be called after all dbus types are registered
         */
        void populateMethodCache();

        ~DBusMgr() override;

        QString introspect(const QString& path) const override;
        bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override;

        /**
         * @return current connected clients
         */
        QList<DBusClientPtr> clients() const;

        /**
         * @return whether the org.freedesktop.secrets service is owned by others
         */
        bool serviceOccupied() const;

        /**
         * Check the running secret service and return info about it
         * @return html string suitable to be shown in the UI
         */
        QString reportExistingService() const;

        // expose on dbus and handle signals
        bool registerObject(Service* service);
        bool registerObject(Collection* coll);
        bool registerObject(Session* sess);
        bool registerObject(Item* item);
        bool registerObject(PromptBase* prompt);

        void unregisterObject(DBusObject* obj);

        // and the signals are handled together with collection's primary path
        bool registerAlias(Collection* coll, const QString& alias);
        void unregisterAlias(const QString& alias);

        /**
         * Return the object path of the pointed DBusObject, or "/" if the pointer is null
         * @tparam T
         * @param object
         * @return
         */
        template <typename T> static QDBusObjectPath objectPathSafe(T* object)
        {
            if (object) {
                return object->objectPath();
            }
            return QDBusObjectPath(QStringLiteral("/"));
        }
        template <typename T> static QDBusObjectPath objectPathSafe(QPointer<T> object)
        {
            return objectPathSafe(object.data());
        }
        static QDBusObjectPath objectPathSafe(std::nullptr_t)
        {
            return QDBusObjectPath(QStringLiteral("/"));
        }

        /**
         * Convert a list of DBusObjects to object path
         * @tparam T
         * @param objects
         * @return
         */
        template <typename T> static QList<QDBusObjectPath> objectsToPath(QList<T*> objects)
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
        template <typename T> T* pathToObject(const QDBusObjectPath& path) const
        {
            if (path.path() == QStringLiteral("/")) {
                return nullptr;
            }
            auto obj = qobject_cast<T*>(m_objects.value(path.path(), nullptr));
            if (!obj) {
                qDebug() << "object not found at path" << path.path();
                qDebug() << m_objects;
            }
            return obj;
        }

        /**
         * Convert a list of object paths to a list of objects.
         * "/" paths (i.e. nullptrs) will be skipped in the resulting list
         * @tparam T
         * @param paths
         * @return
         */
        template <typename T> QList<T*> pathsToObject(const QList<QDBusObjectPath>& paths) const
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

        // Force client to be a specific object, used for testing
        void overrideClient(const DBusClientPtr& fake);

    signals:
        void clientConnected(const DBusClientPtr& client);
        void clientDisconnected(const DBusClientPtr& client);
        void error(const QString& msg);

    private slots:
        void emitCollectionCreated(Collection* coll);
        void emitCollectionChanged(Collection* coll);
        void emitCollectionDeleted(Collection* coll);
        void emitItemCreated(Item* item);
        void emitItemChanged(Item* item);
        void emitItemDeleted(Item* item);
        void emitPromptCompleted(bool dismissed, QVariant result);

        void dbusServiceUnregistered(const QString& service);

    private:
        QDBusConnection m_conn;

        bool serviceInfo(const QString& addr, PeerInfo& info) const;

        bool sendDBusSignal(const QString& path,
                            const QString& interface,
                            const QString& name,
                            const QVariantList& arguments);
        bool sendDBus(const QDBusMessage& reply);

        // object path registration
        QHash<QString, QPointer<DBusObject>> m_objects{};
        enum class PathType
        {
            Service,
            Collection,
            Aliases,
            Prompt,
            Session,
            Item,
            Unknown,
        };
        struct ParsedPath
        {
            PathType type;
            QString id;
            // only used when type == Item
            QString parentId;
            explicit ParsedPath(PathType type = PathType::Unknown, QString id = "", QString parentId = "")
                : type(type)
                , id(std::move(id))
                , parentId(std::move(parentId))
            {
            }
        };
        static ParsedPath parsePath(const QString& path);
        bool registerObject(const QString& path, DBusObject* obj, bool primary = true);

        // method dispatching
        struct MethodData
        {
            int slotIdx{-1};
            QByteArray signature{};
            QVector<int> inputTypes{};
            QVector<int> outputTypes{};
            QVector<int> outputTargetTypes{};
            bool isProperty{false};
            bool needsCallingClient{false};
        };
        QHash<QString, MethodData> m_cachedMethods{};
        void populateMethodCache(const QMetaObject& mo);

        enum class RequestType
        {
            Method,
            PropertyGet,
            PropertyGetAll,
        };
        struct RequestedMethod
        {
            QString interface;
            QString member;
            QString signature;
            QVariantList args;
            RequestType type;
        };
        static bool rewriteRequestForProperty(RequestedMethod& req);
        bool activateObject(const DBusClientPtr& client,
                            const QString& path,
                            const RequestedMethod& req,
                            const QDBusMessage& msg);
        bool objectPropertyGetAll(const DBusClientPtr& client,
                                  DBusObject* obj,
                                  const QString& interface,
                                  const QDBusMessage& msg);
        static bool deliverMethod(const DBusClientPtr& client,
                                  DBusObject* obj,
                                  const MethodData& method,
                                  const QVariantList& args,
                                  DBusResult& ret,
                                  QVariantList& outputArgs);

        // client management
        friend class DBusClient;

        DBusClientPtr findClient(const QString& addr);
        DBusClientPtr createClient(const QString& addr);

        /**
         * @brief This gets called from DBusClient::disconnectDBus
         * @param client
         */
        void removeClient(DBusClient* client);

        QDBusServiceWatcher m_watcher{};
        // mapping from the unique dbus peer address to client object
        QHash<QString, DBusClientPtr> m_clients{};

        DBusClientPtr m_overrideClient;

        friend class ::TestFdoSecrets;
    };
} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSMGR_H
