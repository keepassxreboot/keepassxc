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

#ifndef KEEPASSXC_COMPARE_H
#define KEEPASSXC_COMPARE_H

#include "core/Clock.h"

enum CompareItemOption
{
    CompareItemDefault = 0,
    CompareItemIgnoreMilliseconds = 0x4,
    CompareItemIgnoreStatistics = 0x8,
    CompareItemIgnoreDisabled = 0x10,
    CompareItemIgnoreHistory = 0x20,
    CompareItemIgnoreLocation = 0x40,
};
Q_DECLARE_FLAGS(CompareItemOptions, CompareItemOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(CompareItemOptions)

template <typename Type> inline short compareGeneric(const Type& lhs, const Type& rhs, CompareItemOptions)
{
    if (lhs != rhs) {
        return lhs < rhs ? -1 : +1;
    }
    return 0;
}

template <typename Type>
inline short compare(const Type& lhs, const Type& rhs, CompareItemOptions options = CompareItemDefault)
{
    return compareGeneric(lhs, rhs, options);
}

template <> inline short compare(const QDateTime& lhs, const QDateTime& rhs, CompareItemOptions options)
{
    if (!options.testFlag(CompareItemIgnoreMilliseconds)) {
        return compareGeneric(lhs, rhs, options);
    }
    return compareGeneric(Clock::serialized(lhs), Clock::serialized(rhs), options);
}

template <typename Type>
inline short compare(bool enabled, const Type& lhs, const Type& rhs, CompareItemOptions options = CompareItemDefault)
{
    if (!enabled) {
        return 0;
    }
    return compare(lhs, rhs, options);
}

template <typename Type>
inline short compare(bool lhsEnabled,
                     const Type& lhs,
                     bool rhsEnabled,
                     const Type& rhs,
                     CompareItemOptions options = CompareItemDefault)
{
    const short enabled = compareGeneric(lhsEnabled, rhsEnabled, options);
    if (enabled == 0 && (!options.testFlag(CompareItemIgnoreDisabled) || (lhsEnabled && rhsEnabled))) {
        return compare(lhs, rhs, options);
    }
    return enabled;
}

#endif // KEEPASSX_COMPARE_H
