/*
*  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_YK_CHALLENGERESPONSEKEY_H
#define KEEPASSX_YK_CHALLENGERESPONSEKEY_H

#include "core/Global.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/drivers/YubiKey.h"

class YkChallengeResponseKey : public ChallengeResponseKey
{
public:

    YkChallengeResponseKey(int slot = -1,
                           bool blocking = false);

    QByteArray rawKey() const;
    YkChallengeResponseKey* clone() const;
    bool challenge(const QByteArray& chal);
    bool challenge(const QByteArray& chal, int retries);
    QString getName() const;
    bool isBlocking() const;

private:
    QByteArray m_key;
    int m_slot;
    bool m_blocking;
};

#endif // KEEPASSX_YK_CHALLENGERESPONSEKEY_H
