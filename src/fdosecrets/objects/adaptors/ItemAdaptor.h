/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_ITEMADAPTOR_H
#define KEEPASSXC_FDOSECRETS_ITEMADAPTOR_H

#include "fdosecrets/objects/adaptors/DBusAdaptor.h"

namespace FdoSecrets
{

    class Item;
    class ItemAdaptor : public DBusAdaptor<Item>
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_ITEM)

        Q_PROPERTY(bool Locked READ locked)
        Q_PROPERTY(StringStringMap Attributes READ attributes WRITE setAttributes)
        Q_PROPERTY(QString Label READ label WRITE setLabel)
        Q_PROPERTY(qulonglong Created READ created)
        Q_PROPERTY(qulonglong Modified READ modified)

    public:
        explicit ItemAdaptor(Item* parent);
        ~ItemAdaptor() override = default;

        bool locked() const;

        const StringStringMap attributes() const;
        void setAttributes(const StringStringMap& attrs);

        QString label() const;
        void setLabel(const QString& label);

        qulonglong created() const;

        qulonglong modified() const;

    public slots:
        QDBusObjectPath Delete();
        FdoSecrets::SecretStruct GetSecret(const QDBusObjectPath& session);
        void SetSecret(const FdoSecrets::SecretStruct& secret);
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_ITEMADAPTOR_H
