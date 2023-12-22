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

#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

#include "core/Entry.h"
#include "core/Tools.h"

#ifdef __FreeBSD__
#include <string.h>
#include <sys/sysctl.h>
#endif

namespace FdoSecrets
{
    static const auto IntrospectionService = R"xml(
<interface name="org.freedesktop.Secret.Service">
    <property name="Collections" type="ao" access="read"/>
    <signal name="CollectionCreated">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <signal name="CollectionDeleted">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <signal name="CollectionChanged">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <method name="OpenSession">
        <arg type="v" direction="out"/>
        <arg name="algorithm" type="s" direction="in"/>
        <arg name="input" type="v" direction="in"/>
        <arg name="result" type="o" direction="out"/>
    </method>
    <method name="CreateCollection">
        <arg type="o" direction="out"/>
        <arg name="properties" type="a{sv}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="QVariantMap"/>
        <arg name="alias" type="s" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="SearchItems">
        <arg type="ao" direction="out"/>
        <arg name="attributes" type="a{ss}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="StringStringMap"/>
        <arg name="locked" type="ao" direction="out"/>
    </method>
    <method name="Unlock">
        <arg type="ao" direction="out"/>
        <arg name="paths" type="ao" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="Lock">
        <arg type="ao" direction="out"/>
        <arg name="paths" type="ao" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="GetSecrets">
        <arg type="a{o(oayays)}" direction="out"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="ObjectPathSecretMap"/>
        <arg name="items" type="ao" direction="in"/>
        <arg name="session" type="o" direction="in"/>
    </method>
    <method name="ReadAlias">
        <arg type="o" direction="out"/>
        <arg name="name" type="s" direction="in"/>
    </method>
    <method name="SetAlias">
        <arg name="name" type="s" direction="in"/>
        <arg name="collection" type="o" direction="in"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionCollection = R"xml(
<interface name="org.freedesktop.Secret.Collection">
    <property name="Items" type="ao" access="read"/>
    <property name="Label" type="s" access="readwrite"/>
    <property name="Locked" type="b" access="read"/>
    <property name="Created" type="t" access="read"/>
    <property name="Modified" type="t" access="read"/>
    <signal name="ItemCreated">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <signal name="ItemDeleted">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <signal name="ItemChanged">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <method name="Delete">
        <arg type="o" direction="out"/>
    </method>
    <method name="SearchItems">
        <arg type="ao" direction="out"/>
        <arg name="attributes" type="a{ss}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="StringStringMap"/>
    </method>
    <method name="CreateItem">
        <arg type="o" direction="out"/>
        <arg name="properties" type="a{sv}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="QVariantMap"/>
        <arg name="secret" type="(oayays)" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In1" value="FdoSecrets::wire::Secret"/>
        <arg name="replace" type="b" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionItem = R"xml(
<interface name="org.freedesktop.Secret.Item">
    <property name="Locked" type="b" access="read"/>
    <property name="Attributes" type="a{ss}" access="readwrite">
        <annotation name="org.qtproject.QtDBus.QtTypeName" value="StringStringMap"/>
    </property>
    <property name="Label" type="s" access="readwrite"/>
    <property name="Created" type="t" access="read"/>
    <property name="Modified" type="t" access="read"/>
    <method name="Delete">
        <arg type="o" direction="out"/>
    </method>
    <method name="GetSecret">
        <arg type="(oayays)" direction="out"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="FdoSecrets::wire::Secret"/>
        <arg name="session" type="o" direction="in"/>
    </method>
    <method name="SetSecret">
        <arg name="secret" type="(oayays)" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="FdoSecrets::wire::Secret"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionSession = R"xml(
<interface name="org.freedesktop.Secret.Session">
    <method name="Close">
    </method>
</interface>
)xml";

    static const auto IntrospectionPrompt = R"xml(
<interface name="org.freedesktop.Secret.Prompt">
    <signal name="Completed">
        <arg name="dismissed" type="b" direction="out"/>
        <arg name="result" type="v" direction="out"/>
    </signal>
    <method name="Prompt">
        <arg name="windowId" type="s" direction="in"/>
    </method>
    <method name="Dismiss">
    </method>
</interface>
)xml";

    // read /proc, each field except pid may be empty
    ProcInfo readProc(uint pid)
    {
        ProcInfo info{};
        info.pid = pid;

        // The /proc/pid/exe link is more reliable than /proc/pid/cmdline
        // It's still weak and if the application does a prctl(PR_SET_DUMPABLE, 0) this link cannot be accessed.

#ifdef __FreeBSD__
        const int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, static_cast<int>(info.pid)};
        char buffer[2048];
        size_t size = sizeof(buffer);
        if (sysctl(mib, 4, buffer, &size, NULL, 0) != 0) {
            strlcpy(buffer, "Invalid path", sizeof(buffer));
        }
        QFileInfo exe(buffer);
#else
        QFileInfo exe(QStringLiteral("/proc/%1/exe").arg(pid));
#endif
        info.exePath = exe.canonicalFilePath();

        // /proc/pid/cmdline gives full command line
        QFile cmdline(QStringLiteral("/proc/%1/cmdline").arg(pid));
        if (cmdline.open(QFile::ReadOnly)) {
            info.command = QString::fromLocal8Bit(cmdline.readAll().replace('\0', ' ')).trimmed();
        }

        // /proc/pid/stat gives ppid, name
        QFile stat(QStringLiteral("/proc/%1/stat").arg(pid));
        if (stat.open(QIODevice::ReadOnly)) {
            auto line = stat.readAll();
            // find comm field without looking in what's inside as it's user controlled
            auto commStart = line.indexOf('(');
            auto commEnd = line.lastIndexOf(')');
            if (commStart != -1 && commEnd != -1 && commStart < commEnd) {
                // start past '(', and subtract 2 from total length
                info.name = QString::fromLocal8Bit(line.mid(commStart + 1, commEnd + 1 - commStart - 2));

                auto parts = line.mid(commEnd + 1).trimmed().split(' ');
                if (parts.size() >= 2) {
                    info.ppid = parts[1].toUInt();
                }
            }
        }

        return info;
    }

    DBusMgr::DBusMgr()
        : m_conn(QDBusConnection::sessionBus())
    {
        // remove client when it disappears on the bus
        m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DBusMgr::dbusServiceUnregistered);
        m_watcher.setConnection(m_conn);
    }

    void DBusMgr::populateMethodCache()
    {
        // these are the methods we expose on DBus
        populateMethodCache(Service::staticMetaObject);
        populateMethodCache(Collection::staticMetaObject);
        populateMethodCache(Item::staticMetaObject);
        populateMethodCache(PromptBase::staticMetaObject);
        populateMethodCache(Session::staticMetaObject);
    }

    DBusMgr::~DBusMgr() = default;

    void DBusMgr::overrideClient(const DBusClientPtr& fake)
    {
        m_overrideClient = fake;
    }

    QList<DBusClientPtr> DBusMgr::clients() const
    {
        return m_clients.values();
    }

    bool DBusMgr::serviceInfo(const QString& addr, PeerInfo& info) const
    {
        info.address = addr;
        auto pid = m_conn.interface()->servicePid(addr);
        if (!pid.isValid()) {
            return false;
        }
        info.pid = pid.value();

        // get the whole hierarchy
        uint ppid = info.pid;
        while (ppid != 0) {
            auto proc = readProc(ppid);
            info.hierarchy.append(proc);
            ppid = proc.ppid;
        }

        // check if the exe file is valid
        // (assuming for now that an accessible file is valid)
        info.valid = QFileInfo::exists(info.exePath());

        // ask again to double-check and protect against pid reuse
        auto newPid = m_conn.interface()->servicePid(addr);
        if (!newPid.isValid() || newPid.value() != pid.value()) {
            // either the peer already gone or the pid changed to something else
            return false;
        }
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

        static const QString DBusPathSecrets = DBUS_PATH_SECRETS;

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
        auto reply = m_conn.interface()->isServiceRegistered(DBUS_SERVICE_SECRET);
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

        PeerInfo info{};
        if (serviceInfo(DBUS_SERVICE_SECRET, info)) {
            pidStr = QString::number(info.pid);
            if (!info.exePath().isEmpty()) {
                exeStr = info.exePath();
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
        if (!m_conn.registerService(DBUS_SERVICE_SECRET)) {
            const auto existing = reportExistingService();
            qDebug() << "Failed to register DBus service at " << DBUS_SERVICE_SECRET;
            qDebug() << existing;
            emit error(tr("Failed to register DBus service at %1.<br/>").arg(DBUS_SERVICE_SECRET) + existing);
            return false;
        }
        connect(service, &DBusObject::destroyed, this, [this]() { m_conn.unregisterService(DBUS_SERVICE_SECRET); });

        if (!registerObject(DBUS_PATH_SECRETS, service)) {
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
        auto path = DBUS_PATH_TEMPLATE_COLLECTION.arg(DBUS_PATH_SECRETS, name);
        if (!registerObject(path, coll)) {
            // try again with a suffix
            name.append(QString("_%1").arg(Tools::uuidToHex(QUuid::createUuid()).left(4)));
            path = DBUS_PATH_TEMPLATE_COLLECTION.arg(DBUS_PATH_SECRETS, name);

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
        auto path = DBUS_PATH_TEMPLATE_SESSION.arg(DBUS_PATH_SECRETS, sess->id());
        if (!registerObject(path, sess)) {
            emit error(tr("Failed to register session on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(Item* item)
    {
        auto path = DBUS_PATH_TEMPLATE_ITEM.arg(item->collection()->objectPath().path(), item->backend()->uuidToHex());
        if (!registerObject(path, item)) {
            emit error(tr("Failed to register item on DBus at path '%1'").arg(path));
            return false;
        }
        return true;
    }

    bool DBusMgr::registerObject(PromptBase* prompt)
    {
        auto path = DBUS_PATH_TEMPLATE_PROMPT.arg(DBUS_PATH_SECRETS, Tools::uuidToHex(QUuid::createUuid()));
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
            obj->setObjectPath("/");
        }
    }

    bool DBusMgr::registerAlias(Collection* coll, const QString& alias)
    {
        auto path = DBUS_PATH_TEMPLATE_ALIAS.arg(DBUS_PATH_SECRETS, alias);
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
        auto path = DBUS_PATH_TEMPLATE_ALIAS.arg(DBUS_PATH_SECRETS, alias);
        // DBusMgr::unregisterObject only handles primary path
        m_objects.remove(path);
        m_conn.unregisterObject(path);
    }

    void DBusMgr::emitCollectionCreated(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(DBUS_PATH_SECRETS, DBUS_INTERFACE_SECRET_SERVICE, QStringLiteral("CollectionCreated"), args);
    }

    void DBusMgr::emitCollectionChanged(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(DBUS_PATH_SECRETS, DBUS_INTERFACE_SECRET_SERVICE, "CollectionChanged", args);
    }

    void DBusMgr::emitCollectionDeleted(Collection* coll)
    {
        QVariantList args;
        args += QVariant::fromValue(coll->objectPath());
        sendDBusSignal(DBUS_PATH_SECRETS, DBUS_INTERFACE_SECRET_SERVICE, QStringLiteral("CollectionDeleted"), args);
    }

    void DBusMgr::emitItemCreated(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(
            coll->objectPath().path(), DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemCreated"), args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = DBUS_PATH_TEMPLATE_ALIAS.arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemCreated"), args);
        }
    }

    void DBusMgr::emitItemChanged(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(
            coll->objectPath().path(), DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemChanged"), args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = DBUS_PATH_TEMPLATE_ALIAS.arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemChanged"), args);
        }
    }

    void DBusMgr::emitItemDeleted(Item* item)
    {
        auto coll = item->collection();
        QVariantList args;
        args += QVariant::fromValue(item->objectPath());
        // send on primary path
        sendDBusSignal(
            coll->objectPath().path(), DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemDeleted"), args);
        // also send on all alias path
        for (const auto& alias : coll->aliases()) {
            auto path = DBUS_PATH_TEMPLATE_ALIAS.arg(DBUS_PATH_SECRETS, alias);
            sendDBusSignal(path, DBUS_INTERFACE_SECRET_COLLECTION, QStringLiteral("ItemDeleted"), args);
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
        sendDBusSignal(prompt->objectPath().path(), DBUS_INTERFACE_SECRET_PROMPT, QStringLiteral("Completed"), args);
    }

    DBusClientPtr DBusMgr::findClient(const QString& addr)
    {
        if (m_overrideClient) {
            return m_overrideClient;
        }

        auto it = m_clients.find(addr);
        if (it == m_clients.end()) {
            auto client = createClient(addr);
            if (!client) {
                return {};
            }
            it = m_clients.insert(addr, client);
        }
        return it.value();
    }

    DBusClientPtr DBusMgr::createClient(const QString& addr)
    {
        PeerInfo info{};
        if (!serviceInfo(addr, info)) {
            return {};
        }

        auto client = DBusClientPtr(new DBusClient(this, info));

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

        // client will notify DBusMgr and call DBusMgr::removeClient
        client->disconnectDBus();
    }
} // namespace FdoSecrets
