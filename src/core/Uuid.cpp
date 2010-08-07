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

#include "Uuid.h"

Uuid::Uuid() : m_data(16, 0)
{
}

Uuid::Uuid(bool generate) : m_data(16, 0)
{
    if (generate) {
        while (isNull()) {
            // TODO
        }
    }
}

Uuid::Uuid(const QByteArray& data)
{
    Q_ASSERT(data.size() == 16);

    m_data = data;
}


QString Uuid::toString() const
{
    return m_data.toHex();
}

QByteArray Uuid::toByteArray() const
{
    return m_data;
}

bool Uuid::isNull() const
{
    for (int i=0; i<m_data.size(); ++i) {
        if (m_data[i] != 0)
            return false;
    }

    return true;
}

bool Uuid::operator==(const Uuid& other) const
{
    return m_data == other.m_data;
}

bool Uuid::operator!=(const Uuid& other) const
{
    return !operator==(other);
}

uint qHash(const Uuid& key)
{
    return qHash(key.toByteArray());
}
