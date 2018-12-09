/*
 *  Copyright (C) 2018 Sven Seeberg <mail@sven-seeberg.de>
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

#ifndef KEEPASSX_SMARTCARD_H
#define KEEPASSX_SMARTCARD_H

#include <QSharedPointer>
#include <QString>

#include "keys/Key.h"

class OpenPGPKey : public Key
{
public:
    static QUuid UUID;

    explicit OpenPGPKey(const QString& password);
    QByteArray rawKey() const override;
    void setPassword(const QString& password);

    static QSharedPointer<OpenPGPKey> fromRawKey(const QByteArray& rawKey);
    void addKey();
    void deleteKey2();
    void decrypt();
    void selectKey();

private:
    QByteArray m_key;
};

#endif // KEEPASSX_SMARTCARD_H
