/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
 *  Copyright 2010, Michael Leupold <lemma@confuego.org>
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

#include "DBusTypes.h"

#include <QDBusMetaType>

namespace FdoSecrets
{

    void registerDBusTypes()
    {
        // register meta-types needed for this adaptor
        qRegisterMetaType<SecretStruct>();
        qDBusRegisterMetaType<SecretStruct>();

        qRegisterMetaType<StringStringMap>();
        qDBusRegisterMetaType<StringStringMap>();

        qRegisterMetaType<ObjectPathSecretMap>();
        qDBusRegisterMetaType<ObjectPathSecretMap>();

        QMetaType::registerConverter<QDBusArgument, StringStringMap>([](const QDBusArgument& arg) {
            if (arg.currentSignature() != "a{ss}") {
                return StringStringMap{};
            }
            // QDBusArgument is COW and qdbus_cast modifies it by detaching even it is const.
            // we don't want to modify the instance (arg) stored in the qvariant so we create a copy
            const auto copy = arg; // NOLINT(performance-unnecessary-copy-initialization)
            return qdbus_cast<StringStringMap>(copy);
        });

        // NOTE: this is already registered by Qt in qtextratypes.h
        //     qRegisterMetaType<QList<QDBusObjectPath > >();
        //     qDBusRegisterMetaType<QList<QDBusObjectPath> >();
    }

} // namespace FdoSecrets
