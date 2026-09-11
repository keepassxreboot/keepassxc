/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "PolkitDbusTypes.h"

void PolkitSubject::registerMetaType()
{
    qRegisterMetaType<PolkitSubject>("PolkitSubject");
    qDBusRegisterMetaType<PolkitSubject>();
}

QDBusArgument& operator<<(QDBusArgument& argument, const PolkitSubject& subject)
{
    argument.beginStructure();
    argument << subject.kind << subject.details;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitSubject& subject)
{
    argument.beginStructure();
    argument >> subject.kind >> subject.details;
    argument.endStructure();
    return argument;
}

void PolkitAuthorizationResults::registerMetaType()
{
    qRegisterMetaType<PolkitAuthorizationResults>("PolkitAuthorizationResults");
    qDBusRegisterMetaType<PolkitAuthorizationResults>();
}

QDBusArgument& operator<<(QDBusArgument& argument, const PolkitAuthorizationResults& res)
{
    argument.beginStructure();
    argument << res.is_authorized << res.is_challenge << res.details;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitAuthorizationResults& res)
{
    argument.beginStructure();
    argument >> res.is_authorized >> res.is_challenge >> res.details;
    argument.endStructure();
    return argument;
}

void PolkitActionDescription::registerMetaType()
{
    qRegisterMetaType<PolkitActionDescription>("PolkitActionDescription");
    qDBusRegisterMetaType<PolkitActionDescription>();

    qRegisterMetaType<PolkitActionDescriptionList>("PolkitActionDescriptionList");
    qDBusRegisterMetaType<PolkitActionDescriptionList>();
}

QDBusArgument& operator<<(QDBusArgument& argument, const PolkitActionDescription& action)
{
    argument.beginStructure();
    argument << action.actionId << action.description << action.message << action.vendorName << action.vendorUrl
             << action.iconName << action.implicitAny << action.implicitInactive << action.implicitActive
             << action.annotations;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitActionDescription& action)
{
    argument.beginStructure();
    argument >> action.actionId >> action.description >> action.message >> action.vendorName >> action.vendorUrl
        >> action.iconName >> action.implicitAny >> action.implicitInactive >> action.implicitActive
        >> action.annotations;
    argument.endStructure();
    return argument;
}
