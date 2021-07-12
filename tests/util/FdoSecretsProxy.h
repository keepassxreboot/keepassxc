/*
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETSPROXY_H
#define KEEPASSXC_FDOSECRETSPROXY_H

#include "fdosecrets/dbus/DBusTypes.h"

#include <QtDBus>

/**
 * Mimic the interface of QDBusPendingReply so the same code can be used in test
 */
template <typename T> class PropertyReply
{
    QDBusPendingReply<QDBusVariant> m_reply;

public:
    /*implicit*/ PropertyReply(const QDBusMessage& reply)
        : m_reply(reply)
    {
    }
    bool isFinished() const
    {
        return m_reply.isFinished();
    }
    bool isValid() const
    {
        return m_reply.isValid();
    }
    bool isError() const
    {
        return m_reply.isError();
    }
    QDBusError error() const
    {
        return m_reply.error();
    }
    T value() const
    {
        return qdbus_cast<T>(m_reply.value().variant());
    }
    template <int> T argumentAt() const
    {
        return value();
    }
};

#define IMPL_GET_PROPERTY(name)                                                                                        \
    QDBusMessage msg = QDBusMessage::createMethodCall(                                                                 \
        service(), path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));                  \
    msg << interface() << QStringLiteral(#name);                                                                       \
    return                                                                                                             \
    {                                                                                                                  \
        connection().call(msg, QDBus::BlockWithGui)                                                                    \
    }

#define IMPL_SET_PROPERTY(name, value)                                                                                 \
    QDBusMessage msg = QDBusMessage::createMethodCall(                                                                 \
        service(), path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Set"));                  \
    msg << interface() << QStringLiteral(#name) << QVariant::fromValue(QDBusVariant(QVariant::fromValue(value)));      \
    return                                                                                                             \
    {                                                                                                                  \
        connection().call(msg, QDBus::BlockWithGui)                                                                    \
    }

/*
 * Proxy class for interface org.freedesktop.Secret.Service
 */
class ServiceProxy : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char* staticInterfaceName()
    {
        return "org.freedesktop.Secret.Service";
    }

public:
    ServiceProxy(const QString& service,
                 const QString& path,
                 const QDBusConnection& connection,
                 QObject* parent = nullptr);

    ~ServiceProxy() override;

    inline PropertyReply<QList<QDBusObjectPath>> collections() const
    {
        IMPL_GET_PROPERTY(Collections);
    }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath, QDBusObjectPath> CreateCollection(const QVariantMap& properties,
                                                                                const QString& alias)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(properties) << QVariant::fromValue(alias);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("CreateCollection"), argumentList)};
    }

    inline QDBusPendingReply<FdoSecrets::wire::ObjectPathSecretMap> GetSecrets(const QList<QDBusObjectPath>& items,
                                                                               const QDBusObjectPath& session)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(items) << QVariant::fromValue(session);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("GetSecrets"), argumentList)};
    }

    inline QDBusPendingReply<QList<QDBusObjectPath>, QDBusObjectPath> Lock(const QList<QDBusObjectPath>& paths)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(paths);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Lock"), argumentList)};
    }
    inline QDBusPendingReply<QDBusVariant, QDBusObjectPath> OpenSession(const QString& algorithm,
                                                                        const QDBusVariant& input)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(algorithm) << QVariant::fromValue(input);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("OpenSession"), argumentList)};
    }
    inline QDBusPendingReply<QDBusObjectPath> ReadAlias(const QString& name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("ReadAlias"), argumentList)};
    }

    inline QDBusPendingReply<QList<QDBusObjectPath>, QList<QDBusObjectPath>>
    SearchItems(FdoSecrets::wire::StringStringMap attributes)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(attributes);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("SearchItems"), argumentList)};
    }
    inline QDBusPendingReply<> SetAlias(const QString& name, const QDBusObjectPath& collection)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name) << QVariant::fromValue(collection);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("SetAlias"), argumentList)};
    }

    inline QDBusPendingReply<QList<QDBusObjectPath>, QDBusObjectPath> Unlock(const QList<QDBusObjectPath>& paths)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(paths);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Unlock"), argumentList)};
    }
Q_SIGNALS: // SIGNALS
    void CollectionChanged(const QDBusObjectPath& collection);
    void CollectionCreated(const QDBusObjectPath& collection);
    void CollectionDeleted(const QDBusObjectPath& collection);
};

/*
 * Proxy class for interface org.freedesktop.Secret.Collection
 */
class CollectionProxy : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char* staticInterfaceName()
    {
        return "org.freedesktop.Secret.Collection";
    }

public:
    CollectionProxy(const QString& service,
                    const QString& path,
                    const QDBusConnection& connection,
                    QObject* parent = nullptr);

    ~CollectionProxy() override;

    inline PropertyReply<qulonglong> created() const
    {
        IMPL_GET_PROPERTY(Created);
    }

    inline PropertyReply<QList<QDBusObjectPath>> items() const
    {
        IMPL_GET_PROPERTY(Items);
    }

    inline PropertyReply<QString> label() const
    {
        IMPL_GET_PROPERTY(Label);
    }
    inline QDBusPendingReply<> setLabel(const QString& value)
    {
        IMPL_SET_PROPERTY(Label, value);
    }

    inline PropertyReply<bool> locked() const
    {
        IMPL_GET_PROPERTY(Locked);
    }

    inline PropertyReply<qulonglong> modified() const
    {
        IMPL_GET_PROPERTY(Modified);
    }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath, QDBusObjectPath>
    CreateItem(const QVariantMap& properties, FdoSecrets::wire::Secret secret, bool replace)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(properties) << QVariant::fromValue(secret) << QVariant::fromValue(replace);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("CreateItem"), argumentList)};
    }
    inline QDBusPendingReply<QDBusObjectPath> Delete()
    {
        QList<QVariant> argumentList;
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Delete"), argumentList)};
    }

    inline QDBusPendingReply<QList<QDBusObjectPath>> SearchItems(FdoSecrets::wire::StringStringMap attributes)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(attributes);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("SearchItems"), argumentList)};
    }

Q_SIGNALS: // SIGNALS
    void ItemChanged(const QDBusObjectPath& item);
    void ItemCreated(const QDBusObjectPath& item);
    void ItemDeleted(const QDBusObjectPath& item);
};

/*
 * Proxy class for interface org.freedesktop.Secret.Item
 */
class ItemProxy : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char* staticInterfaceName()
    {
        return "org.freedesktop.Secret.Item";
    }

public:
    ItemProxy(const QString& service,
              const QString& path,
              const QDBusConnection& connection,
              QObject* parent = nullptr);

    ~ItemProxy() override;

    inline PropertyReply<FdoSecrets::wire::StringStringMap> attributes() const
    {
        IMPL_GET_PROPERTY(Attributes);
    }
    inline QDBusPendingReply<> setAttributes(FdoSecrets::wire::StringStringMap value)
    {
        IMPL_SET_PROPERTY(Attributes, value);
    }

    inline PropertyReply<qulonglong> created() const
    {
        IMPL_GET_PROPERTY(Created);
    }

    inline PropertyReply<QString> label() const
    {
        IMPL_GET_PROPERTY(Label);
    }
    inline QDBusPendingReply<> setLabel(const QString& value)
    {
        IMPL_SET_PROPERTY(Label, value);
    }

    inline PropertyReply<bool> locked() const
    {
        IMPL_GET_PROPERTY(Locked);
    }

    inline PropertyReply<qulonglong> modified() const
    {
        IMPL_GET_PROPERTY(Modified);
    }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> Delete()
    {
        QList<QVariant> argumentList;
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Delete"), argumentList)};
    }

    inline QDBusPendingReply<FdoSecrets::wire::Secret> GetSecret(const QDBusObjectPath& session)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(session);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("GetSecret"), argumentList)};
    }

    inline QDBusPendingReply<> SetSecret(FdoSecrets::wire::Secret secret)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(secret);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("SetSecret"), argumentList)};
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface org.freedesktop.Secret.Session
 */
class SessionProxy : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char* staticInterfaceName()
    {
        return "org.freedesktop.Secret.Session";
    }

public:
    SessionProxy(const QString& service,
                 const QString& path,
                 const QDBusConnection& connection,
                 QObject* parent = nullptr);

    ~SessionProxy() override;

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Close()
    {
        QList<QVariant> argumentList;
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Close"), argumentList)};
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface org.freedesktop.Secret.Prompt
 */
class PromptProxy : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char* staticInterfaceName()
    {
        return "org.freedesktop.Secret.Prompt";
    }

public:
    PromptProxy(const QString& service,
                const QString& path,
                const QDBusConnection& connection,
                QObject* parent = nullptr);

    ~PromptProxy() override;

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Dismiss()
    {
        QList<QVariant> argumentList;
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Dismiss"), argumentList)};
    }

    inline QDBusPendingReply<> Prompt(const QString& windowId)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(windowId);
        return {callWithArgumentList(QDBus::BlockWithGui, QStringLiteral("Prompt"), argumentList)};
    }

Q_SIGNALS: // SIGNALS
    void Completed(bool dismissed, const QDBusVariant& result);
};

#undef IMPL_GET_PROPERTY
#undef IMPL_SET_PROPERTY

#endif // KEEPASSXC_FDOSECRETSPROXY_H
