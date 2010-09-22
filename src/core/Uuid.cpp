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

#include <QtCore/QHash>

#include "crypto/Random.h"

const int Uuid::LENGTH = 16;

Uuid::Uuid()
    : m_data(LENGTH, 0)
{
}

Uuid::Uuid(const QByteArray& data)
{
    Q_ASSERT(data.size() == LENGTH);

    m_data = data;
}

Uuid Uuid::random() {
    return Uuid(Random::randomArray(LENGTH));
}

QString Uuid::toBase64() const
{
    return m_data.toBase64();
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

Uuid Uuid::fromBase64(const QString& str)
{
    QByteArray data = QByteArray::fromBase64(str.toAscii());
    return Uuid(data);
}

uint qHash(const Uuid& key)
{
    return qHash(key.toByteArray());
}
