/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_MOCKCHALLENGERESPONSEKEY_H
#define KEEPASSXC_MOCKCHALLENGERESPONSEKEY_H

#include "keys/ChallengeResponseKey.h"

/**
 * Mock challenge-response key implementation that simply
 * returns the challenge concatenated with a fixed secret.
 */
class MockChallengeResponseKey : public ChallengeResponseKey
{
public:
    explicit MockChallengeResponseKey(const QByteArray& secret);
    ~MockChallengeResponseKey() override = default;

    QByteArray rawKey() const override;

    bool challenge(const QByteArray& challenge) override;

private:
    QByteArray m_challenge;
    QByteArray m_secret;

    Q_DISABLE_COPY(MockChallengeResponseKey);
};

#endif // KEEPASSXC_MOCKCHALLENGERESPONSEKEY_H
