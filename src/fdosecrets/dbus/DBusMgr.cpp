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

#include "DBusMgr.h"

#include "fdosecrets/dbus/DBusConstants.h"
#include "fdosecrets/dbus/DBusTypes.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include "core/Entry.h"
#include "core/Global.h"
#include "core/Tools.h"

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDebug>
#include <QFileInfo>
#include <QThread>

#include <utility>

namespace FdoSecrets
{
    DBusMgr::DBusMgr(QDBusConnection conn)
        : m_conn(std::move(conn))
    {
        registerDBusTypes();
        qRegisterMetaType<DBusClientPtr>();
        qRegisterMetaType<DBusClientPtr>("DBusClientPtr");

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

    thread_local DBusClientPtr DBusMgr::Context{};
    const DBusClientPtr& DBusMgr::callingClient() const
    {
        Q_ASSERT(Context);
        return Context;
    }

    void DBusMgr::overrideClient(const DBusClientPtr& fake) const
    {
        Context = fake;
    }

    QList<DBusClientPtr> DBusMgr::clients() const
    {
        return m_clients.values();
    }

    bool DBusMgr::serviceInfo(const QString& addr, ProcessInfo& info) const
    {
        auto pid = m_conn.interface()->servicePid(addr);
        if (!pid.isValid()) {
            return false;
        }
        info.pid = pid.value();
        // The /proc/pid/exe link is more reliable than /proc/pid/cmdline
        // It's still weak and if the application does a prctl(PR_SET_DUMPABLE, 0) this link cannot be accessed.
        QFileInfo proc(QStringLiteral("/proc/%1/exe").arg(pid.value()));
        info.exePath = proc.canonicalFilePath();

        return true;
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
    DBusMgr::ParsedPath DBusMgr::parsePath(const QString& path)
    {
        Q_ASSERT(path.startsWith('/'));
        Q_ASSERT(path == "/" || !path.endsWith('/'));

        static const QString DBusPathSecrets = QStringLiteral(DBUS_PATH_SECRETS);

        if (!path.startsWith(DBusPathSecrets)) {
            return ParsedPath{};
        }
        auto parts = path.mid(DBusPathSecrets.size()).split('/');
        // the first part is always empty
        if (parts.isEmpty() || parts.first() != "") {
            return ParsedPath{};
        }
        parts.takeFirst();

        if (parts.isEmpty()) {
            return ParsedPath{PathType::Service};
        } else if (parts.size() == 2) {
            if (parts.at(0) == "collection") {
                return ParsedPath{PathType::Collection, parts.at(1)};
            } else if (parts.at(0) == "aliases") {
                return ParsedPath{PathType::Aliases, parts.at(1)};
            } else if (parts.at(0) == "prompt") {
                return ParsedPath{PathType::Prompt, parts.at(1)};
            } else if (parts.at(0) == "session") {
                return ParsedPath{PathType::Session, parts.at(1)};
            }
        } else if (parts.size() == 3) {
            if (parts.at(0) == "collection") {
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

        ProcessInfo info{};
        if (serviceInfo(QStringLiteral(DBUS_SERVICE_SECRET), info)) {
            pidStr = QString::number(info.pid);
            if (!info.exePath.isEmpty()) {
                exeStr = info.exePath;
            }
        }

        auto otherService = tr("<i>PID: %1, Executable: %2</i>", "<i>PID: 1234, Executable: /path/to/exe</i>")
                                .arg(pidStr, exeStr.toHtmlEscaped());
        return tr("Another secret service is running (%1).<br/>"
                  "Please stop/remove it before re-enabling the Secret Service Integration.")
            .arg(otherService);
    }

    bool DBusMgr::registerObject(const QString& path, DBusObject* obj, bool primary)
    {
        if (!m_conn.registerVirtualObject(path, this)) {
            qDebug() << "failed to register" << obj << "at" << path;
            return false;
        }
        connect(obj, &DBusObject::destroyed, this, &DBusMgr::unregisterObject);
        m_objects.insert(path, obj);
        if (primary) {
            obj->setObjectPath(path);
        }
        return true;
    }

    bool DBusMgr::registerObject(Service* service)
    {
        if (!m_conn.registerService(QStringLiteral(DBUS_SERVICE_SECRET))) {
            const auto existing = reportExistingService();
            qDebug() << "Failed to register DBus service at " << DBUS_SERVICE_SECRET;
            qDebug() << existing;
            emit error(tr("Failed to register DBus service at %1.<br/>").arg(DBUS_SERVICE_SECRET) + existing);
            return false;
        }
        connect(service, &DBusObject::destroyed, this, [this]() {
            m_conn.unregisterService(QStringLiteral(DBUS_SERVICE_SECRET));
        });

        if (!registerObject(QStringLiteral(DBUS_PATH_SECRETS), service)) {
            qDebug() << "Failed to register service on DBus at path" << DBUS_PATH_SECRETS;
            emit error(tr("Failed to register service on DBus at path '%1'").arg(DBUS_PATH_SECRETS));
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
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_COLLECTION).arg(DBUS_PATH_SECRETS, name);
        if (!registerObject(path, coll)) {

            // try again with a suffix
            name += QStringLiteral("_%1").arg(Tools::uuidToHex(QUuid::createUuid()).left(4));
            path = QStringLiteral(DBUS_PATH_TEMPLATE_COLLECTION).arg(DBUS_PATH_SECRETS, name);

            if (!registerObject(path, coll)) {
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
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_SESSION).arg(DBUS_PATH_SECRETS, sess->id());
        if (!registerObject(path, sess)) {
            emit error(tr("Failed to register session on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(Item* item)
    {
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ITEM)
                        .arg(item->collection()->objectPath().path(), item->backend()->uuidToHex());
        if (!registerObject(path, item)) {
            emit error(tr("Failed to register item on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(PromptBase* prompt)
    {
        auto path =
            QStringLiteral(DBUS_PATH_TEMPLATE_PROMPT).arg(DBUS_PATH_SECRETS, Tools::uuidToHex(QUuid::createUuid()));
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
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(QStringLiteral(DBUS_PATH_SECRETS), alias);
        if (!registerObject(path, coll, false)) {
            qDebug() << "Failed to register database on DBus under alias" << alias;
            // usually this is reported back directly on dbus, so no need to show in UI
            return false;
        }
        // alias signals are handled together with collections' primary path in emitCollection*
        // but we need to handle object destroy here
        connect(coll, &DBusObject::destroyed, this, [this, alias]() { unregisterAlias(alias); });
        return true;
    }

    void DBusMgr::unregisterAlias(const QString& alias)
    {
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(DBUS_PATH_SECRETS, alias);
        // DBusMgr::unregisterObject only handles primary path
        m_objects.remove(path);
        m_conn.unregisterObject(path);
    }

    void DBusMgr::emitCollectionCreated(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(QStringLiteral(DBUS_PATH_SECRETS),
                       QStringLiteral(DBUS_INTERFACE_SECRET_SERVICE),
                       QStringLiteral("CollectionCreated"),
                       args);
    }

    void DBusMgr::emitCollectionChanged(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(QStringLiteral(DBUS_PATH_SECRETS),
                       QStringLiteral(DBUS_INTERFACE_SECRET_SERVICE),
                       "CollectionChanged",
                       args);
    }

    void DBusMgr::emitCollectionDeleted(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(QStringLiteral(DBUS_PATH_SECRETS),
                       QStringLiteral(DBUS_INTERFACE_SECRET_SERVICE),
                       QStringLiteral("CollectionDeleted"),
                       args);
    }

    void DBusMgr::emitItemCreated(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION),
                       QStringLiteral("ItemCreated"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION), QStringLiteral("ItemCreated"), args);
        }
    }

    void DBusMgr::emitItemChanged(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION),
                       QStringLiteral("ItemChanged"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION), QStringLiteral("ItemChanged"), args);
        }
    }

    void DBusMgr::emitItemDeleted(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(coll->objectPath().path(),
                       QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION),
                       QStringLiteral("ItemDeleted"),
                       args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION), QStringLiteral("ItemDeleted"), args);
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
        args += QVariant::fromValue(dismissed);
        args += QVariant::fromValue(QDBusVariant(result));
        sendDBusSignal(prompt->objectPath().path(),
                       QStringLiteral(DBUS_INTERFACE_SECRET_PROMPT),
                       QStringLiteral("Completed"),
                       args);
    }

    DBusClientPtr DBusMgr::findClient(const QString& addr)
    {
        auto it = m_clients.find(addr);
        if (it == m_clients.end()) {
            auto client = createClient(addr);
            if (!client) {
                return {};
            }
            it = m_clients.insert(addr, client);
        }
        // double check the client
        ProcessInfo info{};
        if (!serviceInfo(addr, info) || info.pid != it.value()->pid()) {
            dbusServiceUnregistered(addr);
            return {};
        }
        return it.value();
    }

    DBusClientPtr DBusMgr::createClient(const QString& addr)
    {
        ProcessInfo info{};
        if (!serviceInfo(addr, info)) {
            return {};
        }

        auto client =
            DBusClientPtr(new DBusClient(*this, addr, info.pid, info.exePath.isEmpty() ? addr : info.exePath));

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
