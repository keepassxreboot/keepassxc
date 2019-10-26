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

#ifndef KEEPASSXC_FDOSECRETS_SESSIONADAPTOR_H
#define KEEPASSXC_FDOSECRETS_SESSIONADAPTOR_H

#include "fdosecrets/objects/adaptors/DBusAdaptor.h"

namespace FdoSecrets
{

    class Session;
    class SessionAdaptor : public DBusAdaptor<Session>
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_SESSION)

    public:
        explicit SessionAdaptor(Session* parent);
        ~SessionAdaptor() override = default;

    public slots:
        void Close();
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SESSIONADAPTOR_H
