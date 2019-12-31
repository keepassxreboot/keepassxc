/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TRISTATE_H
#define KEEPASSX_TRISTATE_H

class TriState : public QObject
{
    Q_OBJECT

public:
    enum State
    {
        Inherit,
        Enable,
        Disable
    };

    static bool isValidIndex(int index);
    static int indexFromTriState(State triState);
    static State triStateFromIndex(int index);
};

#endif // KEEPASSX_TRISTATE_H
