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

#ifndef KEEPASSXC_FDOSECRETS_SECRETSERVICEDBUS_H
#define KEEPASSXC_FDOSECRETS_SECRETSERVICEDBUS_H

#include "DBusAdaptor.h"

#include <QDBusObjectPath>

namespace FdoSecrets
{
    /**
     * @brief Adapter class for interface org.freedesktop.Secret.Service
     */
    class Service;
    class ServiceAdaptor : public DBusAdaptor<Service>
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_SERVICE)

        Q_PROPERTY(QList<QDBusObjectPath> Collections READ collections)

    public:
        explicit ServiceAdaptor(Service* parent);
        ~ServiceAdaptor() override = default;

        const QList<QDBusObjectPath> collections() const;

    public slots:
        QDBusVariant OpenSession(const QString& algorithm, const QDBusVariant& input, QDBusObjectPath& result);

        QDBusObjectPath CreateCollection(const QVariantMap& properties, const QString& alias, QDBusObjectPath& prompt);

        const QList<QDBusObjectPath> SearchItems(const StringStringMap& attributes, QList<QDBusObjectPath>& locked);

        const QList<QDBusObjectPath> Unlock(const QList<QDBusObjectPath>& paths, QDBusObjectPath& prompt);

        const QList<QDBusObjectPath> Lock(const QList<QDBusObjectPath>& paths, QDBusObjectPath& prompt);

        const ObjectPathSecretMap GetSecrets(const QList<QDBusObjectPath>& items, const QDBusObjectPath& session);

        QDBusObjectPath ReadAlias(const QString& name);

        void SetAlias(const QString& name, const QDBusObjectPath& collection);

    signals:
        void CollectionCreated(const QDBusObjectPath& collection);

        void CollectionDeleted(const QDBusObjectPath& collection);

        void CollectionChanged(const QDBusObjectPath& collection);
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SECRETSERVICEDBUS_H
