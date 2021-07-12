/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
 *  Copyright 2010, Michael Leupold <lemma@confuego.org>
 *  Copyright 2010-2011, Valentin Rusu <valir@kde.org>
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

#ifndef KEEPASSXC_FDOSECRETS_DBUSTYPES_H
#define KEEPASSXC_FDOSECRETS_DBUSTYPES_H

#include <QDBusArgument>

namespace FdoSecrets
{
    struct Secret;
    class DBusMgr;

    // types used directly in Qt DBus system
    namespace wire
    {
        struct Secret
        {
            QDBusObjectPath session;
            QByteArray parameters;
            QByteArray value;
            QString contentType;

            ::FdoSecrets::Secret unmarshal(const QWeakPointer<DBusMgr>& weak) const;
        };

        inline QDBusArgument& operator<<(QDBusArgument& argument, const Secret& secret)
        {
            argument.beginStructure();
            argument << secret.session << secret.parameters << secret.value << secret.contentType;
            argument.endStructure();
            return argument;
        }

        inline const QDBusArgument& operator>>(const QDBusArgument& argument, Secret& secret)
        {
            argument.beginStructure();
            argument >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
            argument.endStructure();
            return argument;
        }

        using StringStringMap = QMap<QString, QString>;
        using ObjectPathSecretMap = QMap<QDBusObjectPath, Secret>;
    } // namespace wire

    // types used in method parameters
    class Session;
    class Item;
    struct Secret
    {
        const Session* session;
        QByteArray parameters;
        QByteArray value;
        QString contentType;

        wire::Secret marshal() const;
    };
    using wire::StringStringMap;
    using ItemSecretMap = QHash<Item*, Secret>;

    /**
     * Register the types needed for the fd.o Secrets D-Bus interface.
     */
    void registerDBusTypes(const QSharedPointer<DBusMgr>& dbus);

    struct ParamData
    {
        QByteArray signature;
        int dbusTypeId;
    };

    /**
     * @brief Convert parameter type to on-the-wire type and associated dbus signature.
     * This is NOT a generic version, and only handles types used in org.freedesktop.secrets
     * @param id
     * @return ParamData
     */
    ParamData typeToWireType(int id);
} // namespace FdoSecrets

Q_DECLARE_METATYPE(FdoSecrets::wire::Secret)
Q_DECLARE_METATYPE(FdoSecrets::wire::StringStringMap);
Q_DECLARE_METATYPE(FdoSecrets::wire::ObjectPathSecretMap);

Q_DECLARE_METATYPE(FdoSecrets::Secret)

#endif // KEEPASSXC_FDOSECRETS_DBUSTYPES_H
