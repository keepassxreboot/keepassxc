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

#ifndef KEEPASSX_TIMEINFO_H
#define KEEPASSX_TIMEINFO_H

#include <QDateTime>

class TimeInfo
{
public:
    TimeInfo();

    QDateTime lastModificationTime() const;
    QDateTime creationTime() const;
    QDateTime lastAccessTime() const;
    QDateTime expiryTime() const;
    bool expires() const;
    int usageCount() const;
    QDateTime locationChanged() const;

    void setLastModificationTime(const QDateTime& dateTime);
    void setCreationTime(const QDateTime& dateTime);
    void setLastAccessTime(const QDateTime& dateTime);
    void setExpiryTime(const QDateTime& dateTime);
    void setExpires(bool expires);
    void setUsageCount(int count);
    void setLocationChanged(const QDateTime& dateTime);

private:
    QDateTime m_lastModificationTime;
    QDateTime m_creationTime;
    QDateTime m_lastAccessTime;
    QDateTime m_expiryTime;
    bool m_expires;
    int m_usageCount;
    QDateTime m_locationChanged;
};

#endif // KEEPASSX_TIMEINFO_H
