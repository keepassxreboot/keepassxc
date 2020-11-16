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

#ifndef KEEPASSXC_FDOSECRETS_DBUSADAPTOR_H
#define KEEPASSXC_FDOSECRETS_DBUSADAPTOR_H

#include "fdosecrets/objects/DBusReturn.h"
#include "fdosecrets/objects/DBusTypes.h"

#include <QDBusAbstractAdaptor>

namespace FdoSecrets
{

    /**
     * @brief A common adapter class
     */
    template <typename Parent> class DBusAdaptor : public QDBusAbstractAdaptor
    {
    public:
        explicit DBusAdaptor(Parent* parent = nullptr)
            : QDBusAbstractAdaptor(parent)
        {
        }

        ~DBusAdaptor() override = default;

    protected:
        Parent* p() const
        {
            return qobject_cast<Parent*>(parent());
        }
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSADAPTOR_H
