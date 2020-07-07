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

#include "keys/YkChallengeResponseKeyCLI.h"
#include "keys/drivers/YubiKey.h"

#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"

#include <QFile>

QUuid YkChallengeResponseKeyCLI::UUID("e2be77c0-c810-417a-8437-32f41d00bd1d");

YkChallengeResponseKeyCLI::YkChallengeResponseKeyCLI(YubiKeySlot keySlot, QString interactionMessage, QTextStream& out)
    : ChallengeResponseKey(UUID)
    , m_keySlot(keySlot)
    , m_interactionMessage(interactionMessage)
    , m_out(out.device())
{
    connect(YubiKey::instance(), SIGNAL(userInteractionRequest()), SLOT(showInteractionMessage()));
}

void YkChallengeResponseKeyCLI::showInteractionMessage()
{
    m_out << m_interactionMessage << "\n\n" << flush;
}

QByteArray YkChallengeResponseKeyCLI::rawKey() const
{
    return m_key;
}

bool YkChallengeResponseKeyCLI::challenge(const QByteArray& challenge)
{
    auto result = YubiKey::instance()->challenge(m_keySlot, challenge, m_key);
    return result == YubiKey::SUCCESS;
}
