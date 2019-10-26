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

#ifndef KEEPASSXC_FDOSECRETS_COLLECTIONADAPTOR_H
#define KEEPASSXC_FDOSECRETS_COLLECTIONADAPTOR_H

#include "fdosecrets/objects/adaptors/DBusAdaptor.h"

#include <QList>

namespace FdoSecrets
{

    class Collection;
    class CollectionAdaptor : public DBusAdaptor<Collection>
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_COLLECTION)

        Q_PROPERTY(QList<QDBusObjectPath> Items READ items)
        Q_PROPERTY(QString Label READ label WRITE setLabel)
        Q_PROPERTY(bool Locked READ locked)
        Q_PROPERTY(qulonglong Created READ created)
        Q_PROPERTY(qulonglong Modified READ modified)

    public:
        explicit CollectionAdaptor(Collection* parent);
        ~CollectionAdaptor() override = default;

        const QList<QDBusObjectPath> items() const;

        QString label() const;
        void setLabel(const QString& label);

        bool locked() const;

        qulonglong created() const;

        qulonglong modified() const;

    public slots:
        QDBusObjectPath Delete();
        QList<QDBusObjectPath> SearchItems(const StringStringMap& attributes);
        QDBusObjectPath CreateItem(const QVariantMap& properties,
                                   const FdoSecrets::SecretStruct& secret,
                                   bool replace,
                                   QDBusObjectPath& prompt);

    signals:
        void ItemCreated(const QDBusObjectPath& item);
        void ItemDeleted(const QDBusObjectPath& item);
        void ItemChanged(const QDBusObjectPath& item);
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_COLLECTIONADAPTOR_H
