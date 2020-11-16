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
#include <QDBusObjectPath>
#include <QMap>
#include <QString>

#define DBUS_SERVICE_SECRET "org.freedesktop.secrets"

#define DBUS_INTERFACE_SECRET_SERVICE "org.freedesktop.Secret.Service"
#define DBUS_INTERFACE_SECRET_SESSION "org.freedesktop.Secret.Session"
#define DBUS_INTERFACE_SECRET_COLLECTION "org.freedesktop.Secret.Collection"
#define DBUS_INTERFACE_SECRET_ITEM "org.freedesktop.Secret.Item"
#define DBUS_INTERFACE_SECRET_PROMPT "org.freedesktop.Secret.Prompt"

#define DBUS_ERROR_SECRET_NO_SESSION "org.freedesktop.Secret.Error.NoSession"
#define DBUS_ERROR_SECRET_NO_SUCH_OBJECT "org.freedesktop.Secret.Error.NoSuchObject"
#define DBUS_ERROR_SECRET_IS_LOCKED "org.freedesktop.Secret.Error.IsLocked"

#define DBUS_PATH_SECRETS "/org/freedesktop/secrets"

#define DBUS_PATH_TEMPLATE_ALIAS "%1/aliases/%2"
#define DBUS_PATH_TEMPLATE_SESSION "%1/session/%2"
#define DBUS_PATH_TEMPLATE_COLLECTION "%1/collection/%2"
#define DBUS_PATH_TEMPLATE_ITEM "%1/%2"
#define DBUS_PATH_TEMPLATE_PROMPT "%1/prompt/%2"

namespace FdoSecrets
{
    /**
     * This is the basic Secret structure exchanged via the dbus API
     * See the spec for more details
     */
    struct SecretStruct
    {
        QDBusObjectPath session{};
        QByteArray parameters{};
        QByteArray value{};
        QString contentType{};
    };

    inline QDBusArgument& operator<<(QDBusArgument& argument, const SecretStruct& secret)
    {
        argument.beginStructure();
        argument << secret.session << secret.parameters << secret.value << secret.contentType;
        argument.endStructure();
        return argument;
    }

    inline const QDBusArgument& operator>>(const QDBusArgument& argument, SecretStruct& secret)
    {
        argument.beginStructure();
        argument >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
        argument.endStructure();
        return argument;
    }

    /**
     * Register the types needed for the fd.o Secrets D-Bus interface.
     */
    void registerDBusTypes();

} // namespace FdoSecrets

typedef QMap<QString, QString> StringStringMap;
typedef QMap<QDBusObjectPath, FdoSecrets::SecretStruct> ObjectPathSecretMap;

Q_DECLARE_METATYPE(FdoSecrets::SecretStruct)
Q_DECLARE_METATYPE(StringStringMap);
Q_DECLARE_METATYPE(ObjectPathSecretMap);

#endif // KEEPASSXC_FDOSECRETS_DBUSTYPES_H
