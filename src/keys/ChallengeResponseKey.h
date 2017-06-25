/*
*  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_CHALLENGE_RESPONSE_KEY_H
#define KEEPASSX_CHALLENGE_RESPONSE_KEY_H

#include <QByteArray>

class ChallengeResponseKey
{
public:
    virtual ~ChallengeResponseKey() {}
    virtual QByteArray rawKey() const = 0;
    virtual bool challenge(const QByteArray& challenge) = 0;
};

#endif // KEEPASSX_CHALLENGE_RESPONSE_KEY_H
