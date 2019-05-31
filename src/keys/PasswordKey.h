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

#ifndef KEEPASSX_PASSWORDKEY_H
#define KEEPASSX_PASSWORDKEY_H

#include <QSharedPointer>
#include <QString>

#include "keys/Key.h"

class PasswordKey : public Key
{
public:
    static QUuid UUID;

    PasswordKey();
    explicit PasswordKey(const QString& password);
    ~PasswordKey() override;
    QByteArray rawKey() const override;
    void setPassword(const QString& password);

    static QSharedPointer<PasswordKey> fromRawKey(const QByteArray& rawKey);

private:
    static constexpr int SHA256_SIZE = 32;

    char* m_key = nullptr;
};

#endif // KEEPASSX_PASSWORDKEY_H
