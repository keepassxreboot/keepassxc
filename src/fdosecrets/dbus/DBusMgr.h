//
// Created by aetf on 11/15/20.
//

#ifndef KEEPASSXC_FDOSECRETS_DBUSMGR_H
#define KEEPASSXC_FDOSECRETS_DBUSMGR_H

#include <QDBusVirtualObject>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QPointer>
#include <QHash>

#include <utility>

class FdoSecretsPlugin;
namespace FdoSecrets {
    class Collection;
    class Service;
    class PromptBase;
    class Session;
    class Item;
    class DBusObject;

    /**
     * DBusMgr takes care of the interaction between dbus and business logic objects (DBusObject). It handles the following
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
     * The DBusObject invokable methods uses QVariant/DBusObject* and other primitive types in parameters (parameter types).
     * FdoSecrets::typeToWireType establishes the mapping from parameter types to on-the-wire types.
     * The conversion between types is done with the help of QMetaType convert.
     *
     * The method delivery sequence:
     * - DBusMgr::handleMessage unifies method call and property access into the same form
     * - DBusMgr::activateObject finds the target object and calls the method by doing the following
     *   * check the object exists and the interface matches
     *   * find the cached method information MethodData
     *   * DBusMgr::prepareInputParams check and convert input arguments in QDBusMessage::arguments() to types expected by DBusObject
     *   * prepare output argument storage
     *   * call the method
     *   * convert types to what Qt DBus expects
     *
     * The MethodData is pre-computed using Qt meta object system by finding methods with signature matching a certain pattern:
     * Q_INVOKABLE DBusResult methodName(const X& input1, const Y& input2, Z& output1, ZZ& output2)
     */
    class DBusMgr : public QDBusVirtualObject
    {
        Q_OBJECT
    public:
        explicit DBusMgr(QDBusConnection conn);
        ~DBusMgr() override;

        QString introspect(const QString &path) const override;
        bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override;

        /**
         * @brief A thread local instance, only set during handleMessage
         * @return
         */
        static DBusMgr* currentContext();

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
        template <typename T> static T* pathToObject(const QDBusObjectPath& path)
        {
            if (!ThreadLocalInstance) {
                return nullptr;
            }
            if (path.path() == QStringLiteral("/")) {
                return nullptr;
            }
            return qobject_cast<T*>(ThreadLocalInstance->m_objects.value(path.path(), nullptr));
        }

        /**
         * Convert a list of object paths to a list of objects.
         * "/" paths (i.e. nullptrs) will be skipped in the resulting list
         * @tparam T
         * @param paths
         * @return
         */
        template <typename T> QList<T*> static pathsToObject(const QList<QDBusObjectPath>& paths)
        {
            if (!ThreadLocalInstance) {
                return {};
            }

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

    signals:
        void error(const QString& msg);

    private slots:
        void emitCollectionCreated(Collection* coll);
        void emitCollectionChanged(Collection* coll);
        void emitCollectionDeleted(Collection* coll);
        void emitItemCreated(Item* item);
        void emitItemChanged(Item* item);
        void emitItemDeleted(Item* item);
        void emitPromptCompleted(bool dismissed, QVariant result);

    private:
        bool registerObject(const QString& path, DBusObject* obj);

        enum class PathType {
            Service,
            Collection,
            Aliases,
            Prompt,
            Session,
            Item,
            Unknown,
        };
        struct ParsedPath {
            PathType type;
            QString id;
            // only used when type == Item
            QString parentId;
            explicit ParsedPath(PathType type = PathType::Unknown, QString id = "", QString parentId = "")
                : type(type), id(std::move(id)), parentId(std::move(parentId)) {}
        };
        ParsedPath parsePath(const QString& path) const;

        bool sendDBusSignal(const QString& path, const QString& interface, const QString& name, const QVariantList& arguments);
        bool sendDBus(const QDBusMessage& reply);

    private:
        // method dispatching
        struct MethodData
        {
            int slotIdx{-1};
            QByteArray signature{};
            QVector<int> inputTypes{};
            QVector<int> outputTypes{};
            QVector<int> outputTargetTypes{};
        };
        QHash<QString, MethodData> m_cachedMethods;
        void populateMethodCache(const QMetaObject& mo);
        bool activateObject(const QString& path, const QString& interface, const QString& member, const QDBusMessage& msg);

    private:
        QDBusConnection m_conn;

        QHash<QString, QPointer<DBusObject>> m_objects{};

        static thread_local DBusMgr* ThreadLocalInstance;
    };
} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSMGR_H
