/*
 * Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_TESTGLOBAL_H
#define KEEPASSXC_TESTGLOBAL_H

#include "core/Group.h"

#include <QDateTime>
#include <QTest>

namespace QTest
{
  
    template <> inline char* toString(const Group::TriState& triState)
    {
        QString value;

        if (triState == Group::Inherit) {
            value = "null";
        } else if (triState == Group::Enable) {
            value = "true";
        } else {
            value = "false";
        }

        return qstrdup(value.toLocal8Bit().constData());
    }

} // namespace QTest

namespace Test
{

    inline QDateTime datetime(int year, int month, int day, int hour, int min, int second)
    {
        return QDateTime(QDate(year, month, day), QTime(hour, min, second), Qt::UTC);
    }

} // namespace Test

#endif // KEEPASSXC_TESTGLOBAL_H
