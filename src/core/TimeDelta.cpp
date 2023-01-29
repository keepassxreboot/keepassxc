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
    return dateTime.addSecs(delta.getHours() * 3600)
        .addDays(delta.getDays())
        .addMonths(delta.getMonths())
        .addYears(delta.getYears());
}

TimeDelta TimeDelta::fromHours(int hours)
{
    return {hours, 0, 0, 0};
}

TimeDelta TimeDelta::fromDays(int days)
{
    return {0, days, 0, 0};
}

TimeDelta TimeDelta::fromMonths(int months)
{
    return {0, 0, months, 0};
}

TimeDelta TimeDelta::fromYears(int years)
{
    return {0, 0, 0, years};
}

TimeDelta::TimeDelta()
    : m_hours(0)
    , m_days(0)
    , m_months(0)
    , m_years(0)
{
}

TimeDelta::TimeDelta(int hours, int days, int months, int years)
    : m_hours(hours)
    , m_days(days)
    , m_months(months)
    , m_years(years)
{
}

int TimeDelta::getHours() const
{
    return m_hours;
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
