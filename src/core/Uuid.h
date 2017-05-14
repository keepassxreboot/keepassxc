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

#ifndef KEEPASSX_UUID_H
#define KEEPASSX_UUID_H

#include <QByteArray>
#include <QString>
#include <QRegExp>

class Uuid
{
public:
    Uuid();
    explicit Uuid(const QByteArray& data);
    static Uuid random();
    QString toBase64() const;
    QString toHex() const;
    QByteArray toByteArray() const;

    bool isNull() const;
    Uuid& operator=(const Uuid& other);
    bool operator==(const Uuid& other) const;
    bool operator!=(const Uuid& other) const;
    static const int Length;
    static const QRegExp HexRegExp;
    static Uuid fromBase64(const QString& str);
    static Uuid fromHex(const QString& str);
    static bool isUuid(const QString& str);

private:
    QByteArray m_data;
};

Q_DECLARE_TYPEINFO(Uuid, Q_MOVABLE_TYPE);

uint qHash(const Uuid& key);

QDataStream& operator<<(QDataStream& stream, const Uuid& uuid);
QDataStream& operator>>(QDataStream& stream, Uuid& uuid);

#endif // KEEPASSX_UUID_H
