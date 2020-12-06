//
// Created by aetf on 11/15/20.
//

#include "DBusMgr.h"

#include "DBusConstants.h"
#include "DBusTypes.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include "core/Tools.h"
#include "core/Entry.h"
#include "core/Global.h"

#include <QDBusMessage>
#include <QDebug>
#include <QFileInfo>
#include <QThread>
#include <QCoreApplication>

#include <utility>

namespace FdoSecrets
{
    DBusMgr::DBusMgr(QDBusConnection conn)
        : m_conn(std::move(conn))
    {
        // these are the methods we expose on DBus
        populateMethodCache(Service::staticMetaObject);
        populateMethodCache(Collection::staticMetaObject);
        populateMethodCache(Item::staticMetaObject);
        populateMethodCache(PromptBase::staticMetaObject);
        populateMethodCache(Session::staticMetaObject);

        // remove client when it disappears on the bus
        m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DBusMgr::dbusServiceUnregistered);
        m_watcher.setConnection(m_conn);
    }

    DBusMgr::~DBusMgr() = default;

    thread_local DBusClient* DBusMgr::Context = nullptr;
    DBusClient& DBusMgr::callingClient() const
    {
        Q_ASSERT(Context);
        return *Context;
    }

    QList<DBusClientPtr> DBusMgr::clients() const
    {
        return m_clients.values();
    }

    bool DBusMgr::sendDBusSignal(const QString& path,
                                 const QString& interface,
                                 const QString& name,
                                 const QVariantList& arguments)
    {
        auto msg = QDBusMessage::createSignal(path, interface, name);
        msg.setArguments(arguments);
        return sendDBus(msg);
    }

    bool DBusMgr::sendDBus(const QDBusMessage& reply)
    {
        bool ok = m_conn.send(reply);
        if (!ok) {
            qDebug() << "Failed to send on DBus:" << reply;
            emit error(tr("Failed to send reply on DBus"));
        }
        return ok;
    }

    // `this` object is registered at multiple paths:
    // /org/freedesktop/secrets
    // /org/freedesktop/secrets/collection/xxx
    // /org/freedesktop/secrets/collection/xxx/yyy
    // /org/freedesktop/secrets/aliases/xxx
    // /org/freedesktop/secrets/session/xxx
    // /org/freedesktop/secrets/prompt/xxx
    //
    // The path validation is left to Qt, this method only do the minimum
    // required to differentiate the paths.
    DBusMgr::ParsedPath DBusMgr::parsePath(const QString& path) const
    {
        Q_ASSERT(path.startsWith('/'));
        Q_ASSERT(path == QLatin1Literal("/") || !path.endsWith('/'));

        static const QLatin1String DBusPathSecrets = QLatin1Literal(DBUS_PATH_SECRETS);

        if (!path.startsWith(DBusPathSecrets)) {
            return ParsedPath{};
        }
        const auto parts = path.mid(DBusPathSecrets.size()).split('/');

        if (parts.isEmpty()) {
            return ParsedPath{PathType::Service};
        } else if (parts.size() == 2) {
            if (parts.at(0) == QLatin1Literal("collection")) {
                return ParsedPath{PathType::Collection, parts.at(1)};
            } else if (parts.at(0) == QLatin1Literal("aliases")) {
                return ParsedPath{PathType::Aliases, parts.at(1)};
            } else if (parts.at(0) == QLatin1Literal("prompt")) {
                return ParsedPath{PathType::Prompt, parts.at(1)};
            } else if (parts.at(0) == QLatin1Literal("session")) {
                return ParsedPath{PathType::Session, parts.at(1)};
            }
        } else if (parts.size() == 3) {
            if (parts.at(0) == QLatin1Literal("collection")) {
                return ParsedPath{PathType::Item, parts.at(2), parts.at(1)};
            }
        }
        return ParsedPath{};
    }

    QString DBusMgr::introspect(const QString& path) const
    {
        auto parsed = parsePath(path);
        switch (parsed.type) {
        case PathType::Service:
            return IntrospectionService;
        case PathType::Collection:
        case PathType::Aliases:
            return IntrospectionCollection;
        case PathType::Prompt:
            return IntrospectionPrompt;
        case PathType::Session:
            return IntrospectionSession;
        case PathType::Item:
            return IntrospectionItem;
        case PathType::Unknown:
        default:
            return "";
        }
    }

    bool DBusMgr::serviceOccupied() const
    {
        auto reply = m_conn.interface()->isServiceRegistered(QStringLiteral(DBUS_SERVICE_SECRET));
        if (!reply.isValid()) {
            return false;
        }
        if (reply.value()) {
            auto pid = m_conn.interface()->servicePid(DBUS_SERVICE_SECRET);
            if (pid.isValid() && pid.value() != qApp->applicationPid()) {
                return true;
            }
        }
        return false;
    }

    QString DBusMgr::reportExistingService() const
    {
        auto pidStr = tr("Unknown", "Unknown PID");
        auto exeStr = tr("Unknown", "Unknown executable path");

        // try get pid
        auto pid = m_conn.interface()->servicePid(DBUS_SERVICE_SECRET);
        if (pid.isValid()) {
            pidStr = QString::number(pid.value());

            // The /proc/pid/exe link is unforgeable by the application AFAICT.
            // It's still weak and if the application does a prctl(PR_SET_DUMPABLE, 0) this link cannot be accessed.
            QFileInfo proc(QStringLiteral("/proc/%1/exe").arg(pid.value()));
            auto exePath = proc.canonicalFilePath();
            if (!exePath.isEmpty()) {
                exeStr = exePath;
            }
        }
        auto otherService = tr("<i>PID: %1, Executable: %2</i>", "<i>PID: 1234, Executable: /path/to/exe</i>")
                                .arg(pidStr, exeStr.toHtmlEscaped());
        return tr("Another secret service is running (%1).<br/>"
                  "Please stop/remove it before re-enabling the Secret Service Integration.")
            .arg(otherService);
    }

    bool DBusMgr::registerObject(const QString& path, DBusObject* obj)
    {
        if (!m_conn.registerVirtualObject(path, this)) {
            return false;
        }
        connect(obj, &DBusObject::destroyed, this, &DBusMgr::unregisterObject);
        m_objects.insert(path, obj);
        obj->setObjectPath(path);
        return true;
    }

    bool DBusMgr::registerObject(Service* service)
    {
        if (!m_conn.registerService(QLatin1Literal(DBUS_SERVICE_SECRET))) {
            const auto existing = reportExistingService();
            qDebug() << "Failed to register DBus service at " << DBUS_SERVICE_SECRET;
            qDebug() << existing;
            emit error(
                tr("Failed to register DBus service at %1.<br/>").arg(QLatin1String(DBUS_SERVICE_SECRET)) + existing);
            return false;
        }
        connect(service, &DBusObject::destroyed, this, [this]() {
            m_conn.unregisterService(QLatin1Literal(DBUS_SERVICE_SECRET));
        });

        if (!registerObject(QLatin1Literal(DBUS_PATH_SECRETS), service)) {
            qDebug() << "Failed to register service on DBus at path" << DBUS_PATH_SECRETS;
            emit error(
                tr("Failed to register service on DBus at path '%1'").arg(QLatin1Literal(DBUS_PATH_SECRETS)));
            return false;
        }

        connect(service, &Service::collectionCreated, this, &DBusMgr::emitCollectionCreated);
        connect(service, &Service::collectionChanged, this, &DBusMgr::emitCollectionChanged);
        connect(service, &Service::collectionDeleted, this, &DBusMgr::emitCollectionDeleted);

        return true;
    }

    bool DBusMgr::registerObject(Collection* coll)
    {
        auto name = encodePath(coll->name());
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_COLLECTION).arg(QLatin1Literal(DBUS_PATH_SECRETS), name);
        if (!registerObject(path, coll)) {
            // try again with a suffix
            name += QLatin1Literal("_%1").arg(Tools::uuidToHex(QUuid::createUuid()).left(4));
            path = QLatin1Literal(DBUS_PATH_TEMPLATE_COLLECTION).arg(QLatin1Literal(DBUS_PATH_SECRETS), name);

            if (!m_conn.registerVirtualObject(path, this)) {
                qDebug() << "Failed to register database on DBus under name" << name;
                emit error(tr("Failed to register database on DBus under the name '%1'").arg(name));
                return false;
            }
        }

        connect(coll, &Collection::itemCreated, this, &DBusMgr::emitItemCreated);
        connect(coll, &Collection::itemChanged, this, &DBusMgr::emitItemChanged);
        connect(coll, &Collection::itemDeleted, this, &DBusMgr::emitItemDeleted);

        return true;
    }

    bool DBusMgr::registerObject(Session* sess)
    {
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_SESSION).arg(QLatin1Literal(DBUS_PATH_SECRETS), sess->id());
        if (!registerObject(path, sess)) {
            emit error(tr("Failed to register session on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(Item* item)
    {
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ITEM)
                        .arg(item->collection()->objectPath().path(), item->backend()->uuidToHex());
        if (!registerObject(path, item)) {
            emit error(tr("Failed to register item on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(PromptBase* prompt)
    {
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_PROMPT)
                        .arg(QLatin1Literal(DBUS_PATH_SECRETS), Tools::uuidToHex(QUuid::createUuid()));
        if (!registerObject(path, prompt)) {
            emit error(tr("Failed to register prompt object on DBus at path '%1'").arg(path));
            return false;
        }

        connect(prompt, &PromptBase::completed, this, &DBusMgr::emitPromptCompleted);

        return true;
    }

    void DBusMgr::unregisterObject(DBusObject* obj)
    {
        auto count = m_objects.remove(obj->objectPath().path());
        if (count > 0) {
            m_conn.unregisterObject(obj->objectPath().path());
            obj->setObjectPath(QStringLiteral("/"));
        }
    }

    bool DBusMgr::registerAlias(Collection* coll, const QString& alias)
    {
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ALIAS).arg(QLatin1Literal(DBUS_PATH_SECRETS), alias);
        if (!registerObject(path, coll)) {
            qDebug() << "Failed to register database on DBus under alias" << alias;
            // usually this is reported back directly on dbus, so no need to show in UI
            return false;
        }
        // alias signals are handled together with collections' primary path in emitCollection*
        return true;
    }

    void DBusMgr::unregisterAlias(const QString& alias)
    {
        auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ALIAS).arg(QLatin1Literal(DBUS_PATH_SECRETS), alias);
        // DBusMgr::unregisterObject only handles primary path
        m_objects.remove(path);
        m_conn.unregisterObject(path);
    }

    void DBusMgr::emitCollectionCreated(Collection* coll)
    {
        QVariantList args;
        args += coll->objectPath();
        sendDBusSignal(QLatin1Literal(DBUS_PATH_SECRETS),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_SERVICE),
                       QLatin1Literal("CollectionCreated"),
                       args);
    }

    void DBusMgr::emitCollectionChanged(Collection* coll)
    {
        QVariantList args;
        args += coll->objectPath();
        sendDBusSignal(QLatin1Literal(DBUS_PATH_SECRETS),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_SERVICE),
                       QLatin1Literal("CollectionChanged"),
                       args);
    }

    void DBusMgr::emitCollectionDeleted(Collection* coll)
    {
        QVariantList args;
        args += coll->objectPath();
        sendDBusSignal(QLatin1Literal(DBUS_PATH_SECRETS),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_SERVICE),
                       QLatin1Literal("CollectionDeleted"),
                       args);
    }

    void DBusMgr::emitItemCreated(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += item->objectPath();
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION),
                       QLatin1Literal("ItemCreated"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ALIAS).arg(QLatin1Literal(DBUS_PATH_SECRETS), alias);
            sendDBusSignal(path, QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION), QLatin1Literal("ItemCreated"), args);
        }
    }

    void DBusMgr::emitItemChanged(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += item->objectPath();
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION),
                       QLatin1Literal("ItemChanged"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ALIAS).arg(QLatin1Literal(DBUS_PATH_SECRETS), alias);
            sendDBusSignal(path, QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION), QLatin1Literal("ItemChanged"), args);
        }
    }

    void DBusMgr::emitItemDeleted(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += item->objectPath();
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION),
                       QLatin1Literal("ItemDeleted"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QLatin1Literal(DBUS_PATH_TEMPLATE_ALIAS).arg(QLatin1Literal(DBUS_PATH_SECRETS), alias);
            sendDBusSignal(path, QLatin1Literal(DBUS_INTERFACE_SECRET_COLLECTION), QLatin1Literal("ItemDeleted"), args);
        }
    }

    void DBusMgr::emitPromptCompleted(bool dismissed, QVariant result)
    {
        auto prompt = qobject_cast<PromptBase*>(sender());
        if (!prompt) {
            qDebug() << "Wrong sender in emitPromptCompleted";
            return;
        }

        // make sure the result contains a valid value, otherwise QDBusVariant refuses to marshall it.
        if (!result.isValid()) {
            result = QString{};
        }

        QVariantList args;
        args += dismissed;
        args += QVariant::fromValue(QDBusVariant(result));
        sendDBusSignal(prompt->objectPath().path(),
                       QLatin1Literal(DBUS_INTERFACE_SECRET_PROMPT),
                       QLatin1Literal("Completed"),
                       args);
    }

    DBusClientPtr DBusMgr::findClient(const QString& addr)
    {
        auto it = m_clients.find(addr);
        if (it == m_clients.end()) {
            it = m_clients.insert(addr, createClient(addr));
        }
        return it.value();
    }

    DBusClientPtr DBusMgr::createClient(const QString& addr)
    {
        auto pid = m_conn.interface()->servicePid(addr);
        auto name = addr;
        if (pid.isValid()) {
            // The /proc/pid/exe link is unforgeable by the application AFAICT.
            // It's still weak and if the application does a prctl(PR_SET_DUMPABLE, 0) this link cannot be accessed.
            QFileInfo proc(QStringLiteral("/proc/%1/exe").arg(pid.value()));
            auto exePath = proc.canonicalFilePath();
            if (!exePath.isEmpty()) {
                name = exePath;
            }
        }

        auto client = DBusClientPtr(new DBusClient(*this, addr, pid.value(), name));

        emit clientConnected(client);
        m_watcher.addWatchedService(addr);

        return client;
    }

    void DBusMgr::removeClient(DBusClient* client)
    {
        if (!client) {
            return;
        }

        auto it = m_clients.find(client->address());
        if (it == m_clients.end()) {
            return;
        }

        emit clientDisconnected(*it);
        m_clients.erase(it);
    }

    void DBusMgr::dbusServiceUnregistered(const QString& service)
    {
        auto removed = m_watcher.removeWatchedService(service);
        if (!removed) {
            qDebug("FdoSecrets: Failed to remove service watcher");
        }

        auto it = m_clients.find(service);
        if (it == m_clients.end()) {
            return;
        }
        auto client = it.value();

        client->disconnectDBus();
    }
} // namespace FdoSecrets
