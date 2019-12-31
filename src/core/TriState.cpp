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

#include <QObject>

#include "TriState.h"

bool TriState::isValidIndex(int index)
{
    return index >= 0 && index <= 2;
}

int TriState::indexFromTriState(TriState::State triState)
{
    switch (triState) {
    case TriState::Inherit:
        return 0;
    case TriState::Enable:
        return 1;
    case TriState::Disable:
        return 2;
    default:
        Q_ASSERT(false);
        return 0;
    }
}

TriState::State TriState::triStateFromIndex(int index)
{
    switch (index) {
    case 0:
        return TriState::Inherit;
    case 1:
        return TriState::Enable;
    case 2:
        return TriState::Disable;
    default:
        Q_ASSERT(false);
        return TriState::Inherit;
    }
}
