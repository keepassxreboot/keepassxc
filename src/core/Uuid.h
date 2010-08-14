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

#include <QtCore/QByteArray>
#include <QtCore/QString>

class Uuid
{
public:
    Uuid();
    explicit Uuid(const QByteArray& data);
    static Uuid random();
    QString toBase64() const;
    QByteArray toByteArray() const;
    bool isNull() const;
    bool operator==(const Uuid& other) const;
    bool operator!=(const Uuid& other) const;
    static const int length;

private:
    QByteArray m_data;
};

uint qHash(const Uuid& key);

#endif // KEEPASSX_UUID_H
