/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TimeInfo.h"

TimeInfo::TimeInfo()
{
    m_expires = false;
}

QDateTime TimeInfo::lastModificationTime() const
{
    return m_lastModificationTime;
}

QDateTime TimeInfo::creationTime() const
{
    return m_creationTime;
}

QDateTime TimeInfo::lastAccessTime() const
{
    return m_lastAccessTime;
}

QDateTime TimeInfo::expiryTime() const
{
    return m_expiryTime;
}

bool TimeInfo::expires() const
{
    return m_expires;
}

int TimeInfo::usageCount() const
{
    return m_usageCount;
}
QDateTime TimeInfo::locationChanged() const
{
    return m_locationChanged;
}

void TimeInfo::setLastModificationTime(const QDateTime& dateTime)
{
    m_lastModificationTime = dateTime;
}

void TimeInfo::setCreationTime(const QDateTime& dateTime)
{
    m_creationTime = dateTime;
}

void TimeInfo::setLastAccessTime(const QDateTime& dateTime)
{
    m_lastAccessTime = dateTime;
}

void TimeInfo::setExpiryTime(const QDateTime& dateTime)
{
    m_expiryTime = dateTime;
}

void TimeInfo::setExpires(bool expires)
{
    m_expires = expires;
}

void TimeInfo::setUsageCount(int count)
{
    m_usageCount = count;
}

void TimeInfo::setLocationChanged(const QDateTime& dateTime)
{
    m_locationChanged = dateTime;
}
