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

#ifndef KEEPASSXC_CLOCK_H
#define KEEPASSXC_CLOCK_H

#include <QDateTime>
#include <QSharedPointer>

class Clock
{
public:
    static QDateTime currentDateTimeUtc();
    static QDateTime currentDateTime();

    static uint currentSecondsSinceEpoch();
    static qint64 currentMilliSecondsSinceEpoch();

    static QDateTime serialized(const QDateTime& dateTime);

    static QDateTime datetimeUtc(int year, int month, int day, int hour, int min, int second);
    static QDateTime datetime(int year, int month, int day, int hour, int min, int second);

    static QDateTime datetimeUtc(qint64 msecSinceEpoch);
    static QDateTime datetime(qint64 msecSinceEpoch);

    static QDateTime parse(const QString& text, Qt::DateFormat format = Qt::TextDate);
    static QDateTime parse(const QString& text, const QString& format);

    virtual ~Clock();

protected:
    Clock();
    virtual QDateTime currentDateTimeUtcImpl() const;
    virtual QDateTime currentDateTimeImpl() const;

    static void resetInstance();
    static void setInstance(Clock* clock);
    static const Clock& instance();

private:
    static QSharedPointer<Clock> m_instance;
};

#endif // KEEPASSX_ENTRY_H
