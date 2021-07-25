/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "YubiKeyInterfacePCSC.h"

YubiKeyInterfacePCSC::YubiKeyInterfacePCSC()
    : YubiKeyInterface()
{
}

YubiKeyInterfacePCSC::~YubiKeyInterfacePCSC()
{
}

YubiKeyInterfacePCSC* YubiKeyInterfacePCSC::m_instance(nullptr);

YubiKeyInterfacePCSC* YubiKeyInterfacePCSC::instance()
{
    if (!m_instance) {
        m_instance = new YubiKeyInterfacePCSC();
    }

    return m_instance;
}

void YubiKeyInterfacePCSC::findValidKeys()
{
}

YubiKey::ChallengeResult
YubiKeyInterfacePCSC::challenge(YubiKeySlot slot, const QByteArray& chal, Botan::secure_vector<char>& resp)
{
    Q_UNUSED(slot);
    Q_UNUSED(chal);
    Q_UNUSED(resp);
    return YubiKey::ChallengeResult::YCR_ERROR;
}

bool YubiKeyInterfacePCSC::testChallenge(YubiKeySlot slot, bool* wouldBlock)
{
    Q_UNUSED(slot);
    Q_UNUSED(wouldBlock);
    return false;
}

YubiKey::ChallengeResult YubiKeyInterfacePCSC::performChallenge(void* key,
                                                                int slot,
                                                                bool mayBlock,
                                                                const QByteArray& challenge,
                                                                Botan::secure_vector<char>& response)
{
    Q_UNUSED(key);
    Q_UNUSED(slot);
    Q_UNUSED(mayBlock);
    Q_UNUSED(challenge);
    Q_UNUSED(response);
    return YubiKey::ChallengeResult::YCR_ERROR;
}

bool YubiKeyInterfacePCSC::performTestChallenge(void* key, int slot, bool* wouldBlock)
{
    Q_UNUSED(key);
    Q_UNUSED(slot);
    Q_UNUSED(wouldBlock);
    return false;
}