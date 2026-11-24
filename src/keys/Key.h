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

#include <QByteArray>
#include <QUuid>

#include <botan/mem_ops.h>

/**
 * Securely wipe a byte array that held key material (e.g. serialized keys) before it is released.
 * Only the buffer owned by @p data is wiped; any other implicitly shared copy is not touched.
 */
inline void scrubKeyData(QByteArray& data)
{
    if (!data.isEmpty()) {
        Botan::secure_scrub_memory(data.data(), static_cast<size_t>(data.size()));
    }
    data.clear();
}

class Key
{
public:
    explicit Key(const QUuid& uuid)
        : m_uuid(uuid) {};
    Q_DISABLE_COPY(Key);
    virtual ~Key() = default;
    virtual QByteArray rawKey() const = 0;
    virtual void setRawKey(const QByteArray& data) = 0;
    virtual QByteArray serialize() const = 0;
    virtual void deserialize(const QByteArray& data) = 0;
    inline virtual QUuid uuid() const
    {
        return m_uuid;
    }

private:
    QUuid m_uuid;
};

#endif // KEEPASSX_KEY_H
