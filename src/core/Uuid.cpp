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

#include <QHash>

#include "crypto/Random.h"

const int Uuid::Length = 16;
const QRegExp Uuid::HexRegExp = QRegExp(QString("^[0-9A-F]{%1}$").arg(QString::number(Uuid::Length * 2)),
                                        Qt::CaseInsensitive);

Uuid::Uuid()
    : m_data(Length, 0)
{
}

Uuid::Uuid(const QByteArray& data)
{
    Q_ASSERT(data.size() == Length);

    m_data = data;
}

Uuid Uuid::random()
{
    return Uuid(randomGen()->randomArray(Length));
}

QString Uuid::toBase64() const
{
    return QString::fromLatin1(m_data.toBase64());
}

QString Uuid::toHex() const
{
    return QString::fromLatin1(m_data.toHex());
}

QByteArray Uuid::toByteArray() const
{
    return m_data;
}

bool Uuid::isNull() const
{
    for (int i = 0; i < m_data.size(); ++i) {
        if (m_data[i] != 0) {
            return false;
        }
    }

    return true;
}

Uuid& Uuid::operator=(const Uuid& other)
{
    m_data = other.m_data;

    return *this;
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
    QByteArray data = QByteArray::fromBase64(str.toLatin1());
    if (data.size() == Uuid::Length) {
        return Uuid(data);
    }
    return {};
}

Uuid Uuid::fromHex(const QString& str)
{
    QByteArray data = QByteArray::fromHex(str.toLatin1());
    if (data.size() == Uuid::Length) {
        return Uuid(data);
    }
    return {};
}

uint qHash(const Uuid& key)
{
    return qHash(key.toByteArray());
}

QDataStream& operator<<(QDataStream& stream, const Uuid& uuid)
{
    return stream << uuid.toByteArray();
}

QDataStream& operator>>(QDataStream& stream, Uuid& uuid)
{
    QByteArray data;
    stream >> data;
    if (data.size() == Uuid::Length) {
        uuid = Uuid(data);
    }

    return stream;
}

bool Uuid::isUuid(const QString& uuid)
{
  return Uuid::HexRegExp.exactMatch(uuid);
}
