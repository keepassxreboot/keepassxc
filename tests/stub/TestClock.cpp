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

#include "TestClock.h"

TestClock::TestClock(int year, int month, int day, int hour, int min, int second)
    : Clock()
    , m_utcCurrent(datetimeUtc(year, month, day, hour, min, second))
{
}

TestClock::TestClock(QDateTime utcBase)
    : Clock()
    , m_utcCurrent(utcBase)
{
}

const QDateTime& TestClock::advanceSecond(int seconds)
{
    m_utcCurrent = m_utcCurrent.addSecs(seconds);
    return m_utcCurrent;
}

const QDateTime& TestClock::advanceMinute(int minutes)
{
    m_utcCurrent = m_utcCurrent.addSecs(minutes * 60);
    return m_utcCurrent;
}

const QDateTime& TestClock::advanceHour(int hours)
{
    m_utcCurrent = m_utcCurrent.addSecs(hours * 60 * 60);
    return m_utcCurrent;
}

const QDateTime& TestClock::advanceDay(int days)
{
    m_utcCurrent = m_utcCurrent.addDays(days);
    return m_utcCurrent;
}

const QDateTime& TestClock::advanceMonth(int months)
{
    m_utcCurrent = m_utcCurrent.addMonths(months);
    return m_utcCurrent;
}

const QDateTime& TestClock::advanceYear(int years)
{
    m_utcCurrent = m_utcCurrent.addYears(years);
    return m_utcCurrent;
}

void TestClock::setup(Clock* clock)
{
    Clock::setInstance(clock);
}

void TestClock::teardown()
{
    Clock::resetInstance();
}

QDateTime TestClock::currentDateTimeUtcImpl() const
{
    return m_utcCurrent;
}

QDateTime TestClock::currentDateTimeImpl() const
{
    return m_utcCurrent.toLocalTime();
}
