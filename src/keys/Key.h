/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_KEY_H
#define KEEPASSX_KEY_H

#include <QUuid>

class Key
{
public:
    explicit Key(const QUuid& uuid)
        : m_uuid(uuid){};
    Q_DISABLE_COPY(Key);
    virtual ~Key() = default;
    virtual QByteArray rawKey() const = 0;
    inline virtual QUuid uuid() const
    {
        return m_uuid;
    }

private:
    QUuid m_uuid;
};

#endif // KEEPASSX_KEY_H
