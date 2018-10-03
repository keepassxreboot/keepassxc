/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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
#include "Compare.h"

#include <QColor>

bool operator<(const QColor& lhs, const QColor& rhs)
{
    const QColor adaptedLhs = lhs.toCmyk();
    const QColor adaptedRhs = rhs.toCmyk();
    const int iCyan = compare(adaptedLhs.cyanF(), adaptedRhs.cyanF());
    if (iCyan != 0) {
        return iCyan;
    }
    const int iMagenta = compare(adaptedLhs.magentaF(), adaptedRhs.magentaF());
    if (iMagenta != 0) {
        return iMagenta;
    }
    const int iYellow = compare(adaptedLhs.yellowF(), adaptedRhs.yellowF());
    if (iYellow != 0) {
        return iYellow;
    }
    return compare(adaptedLhs.blackF(), adaptedRhs.blackF()) < 0;
}
