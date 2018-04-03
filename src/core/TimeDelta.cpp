/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "TimeDelta.h"

#include <QDateTime>

QDateTime operator+(const QDateTime& dateTime, const TimeDelta& delta)
{
    return dateTime.addDays(delta.getDays()).addMonths(delta.getMonths()).addYears(delta.getYears());
}

TimeDelta TimeDelta::fromDays(int days)
{
    return TimeDelta(days, 0, 0);
}

TimeDelta TimeDelta::fromMonths(int months)
{
    return TimeDelta(0, months, 0);
}

TimeDelta TimeDelta::fromYears(int years)
{
    return TimeDelta(0, 0, years);
}

TimeDelta::TimeDelta()
    : m_days(0)
    , m_months(0)
    , m_years(0)
{
}

TimeDelta::TimeDelta(int days, int months, int years)
    : m_days(days)
    , m_months(months)
    , m_years(years)
{
}

int TimeDelta::getDays() const
{
    return m_days;
}

int TimeDelta::getMonths() const
{
    return m_months;
}

int TimeDelta::getYears() const
{
    return m_years;
}
