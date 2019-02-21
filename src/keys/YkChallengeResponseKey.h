/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include <QObject>

class YkChallengeResponseKey : public QObject, public ChallengeResponseKey
{
    Q_OBJECT

public:
    static QUuid UUID;

    explicit YkChallengeResponseKey(int slot = -1, bool blocking = false);
    ~YkChallengeResponseKey() override;

    QByteArray rawKey() const override;
    bool challenge(const QByteArray& challenge) override;
    bool challenge(const QByteArray& challenge, unsigned int retries);
    QString getName() const;
    bool isBlocking() const;

signals:
    /**
     * Emitted whenever user interaction is required to proceed with the challenge-response protocol.
     * You can use this to show a helpful dialog informing the user that his assistance is required.
     */
    void userInteractionRequired();

    /**
     * Emitted when the user has provided their required input.
     */
    void userConfirmed();

private:
    char* m_key = nullptr;
    std::size_t m_keySize = 0;
    int m_slot;
    bool m_blocking;
};

#endif // KEEPASSX_YK_CHALLENGERESPONSEKEY_H
