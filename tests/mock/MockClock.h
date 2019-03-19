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

#ifndef KEEPASSXC_TESTCLOCK_H
#define KEEPASSXC_TESTCLOCK_H

#include "core/Clock.h"

#include <QDateTime>

class MockClock : public Clock
{
public:
    MockClock(int year, int month, int day, int hour, int min, int second);

    MockClock(QDateTime utcBase = QDateTime::currentDateTimeUtc());

    const QDateTime& advanceSecond(int seconds);
    const QDateTime& advanceMinute(int minutes);
    const QDateTime& advanceHour(int hours);
    const QDateTime& advanceDay(int days);
    const QDateTime& advanceMonth(int months);
    const QDateTime& advanceYear(int years);

    static void setup(Clock* clock);
    static void teardown();

protected:
    QDateTime currentDateTimeUtcImpl() const;
    QDateTime currentDateTimeImpl() const;

private:
    QDateTime m_utcCurrent;
};

#endif // KEEPASSXC_TESTCLOCK_H
