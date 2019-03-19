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

#include "MockClock.h"

MockClock::MockClock(int year, int month, int day, int hour, int min, int second)
    : Clock()
    , m_utcCurrent(datetimeUtc(year, month, day, hour, min, second))
{
}

MockClock::MockClock(QDateTime utcBase)
    : Clock()
    , m_utcCurrent(utcBase)
{
}

const QDateTime& MockClock::advanceSecond(int seconds)
{
    m_utcCurrent = m_utcCurrent.addSecs(seconds);
    return m_utcCurrent;
}

const QDateTime& MockClock::advanceMinute(int minutes)
{
    m_utcCurrent = m_utcCurrent.addSecs(minutes * 60);
    return m_utcCurrent;
}

const QDateTime& MockClock::advanceHour(int hours)
{
    m_utcCurrent = m_utcCurrent.addSecs(hours * 60 * 60);
    return m_utcCurrent;
}

const QDateTime& MockClock::advanceDay(int days)
{
    m_utcCurrent = m_utcCurrent.addDays(days);
    return m_utcCurrent;
}

const QDateTime& MockClock::advanceMonth(int months)
{
    m_utcCurrent = m_utcCurrent.addMonths(months);
    return m_utcCurrent;
}

const QDateTime& MockClock::advanceYear(int years)
{
    m_utcCurrent = m_utcCurrent.addYears(years);
    return m_utcCurrent;
}

void MockClock::setup(Clock* clock)
{
    Clock::setInstance(clock);
}

void MockClock::teardown()
{
    Clock::resetInstance();
}

QDateTime MockClock::currentDateTimeUtcImpl() const
{
    return m_utcCurrent;
}

QDateTime MockClock::currentDateTimeImpl() const
{
    return m_utcCurrent.toLocalTime();
}
