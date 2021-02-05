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

#ifndef KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H
#define KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H

#include <QString>

static const auto DBUS_SERVICE_SECRET = QStringLiteral("org.freedesktop.secrets");

#define DBUS_INTERFACE_SECRET_SERVICE_LITERAL "org.freedesktop.Secret.Service"
#define DBUS_INTERFACE_SECRET_SESSION_LITERAL "org.freedesktop.Secret.Session"
#define DBUS_INTERFACE_SECRET_COLLECTION_LITERAL "org.freedesktop.Secret.Collection"
#define DBUS_INTERFACE_SECRET_ITEM_LITERAL "org.freedesktop.Secret.Item"
#define DBUS_INTERFACE_SECRET_PROMPT_LITERAL "org.freedesktop.Secret.Prompt"

static const auto DBUS_INTERFACE_SECRET_SERVICE = QStringLiteral(DBUS_INTERFACE_SECRET_SERVICE_LITERAL);
static const auto DBUS_INTERFACE_SECRET_SESSION = QStringLiteral(DBUS_INTERFACE_SECRET_SESSION_LITERAL);
static const auto DBUS_INTERFACE_SECRET_COLLECTION = QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION_LITERAL);
static const auto DBUS_INTERFACE_SECRET_ITEM = QStringLiteral(DBUS_INTERFACE_SECRET_ITEM_LITERAL);
static const auto DBUS_INTERFACE_SECRET_PROMPT = QStringLiteral(DBUS_INTERFACE_SECRET_PROMPT_LITERAL);

static const auto DBUS_ERROR_SECRET_NO_SESSION = QStringLiteral("org.freedesktop.Secret.Error.NoSession");
static const auto DBUS_ERROR_SECRET_NO_SUCH_OBJECT = QStringLiteral("org.freedesktop.Secret.Error.NoSuchObject");
static const auto DBUS_ERROR_SECRET_IS_LOCKED = QStringLiteral("org.freedesktop.Secret.Error.IsLocked");

static const auto DBUS_PATH_SECRETS = QStringLiteral("/org/freedesktop/secrets");

static const auto DBUS_PATH_TEMPLATE_ALIAS = QStringLiteral("%1/aliases/%2");
static const auto DBUS_PATH_TEMPLATE_SESSION = QStringLiteral("%1/session/%2");
static const auto DBUS_PATH_TEMPLATE_COLLECTION = QStringLiteral("%1/collection/%2");
static const auto DBUS_PATH_TEMPLATE_ITEM = QStringLiteral("%1/%2");
static const auto DBUS_PATH_TEMPLATE_PROMPT = QStringLiteral("%1/prompt/%2");

#endif // KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H
