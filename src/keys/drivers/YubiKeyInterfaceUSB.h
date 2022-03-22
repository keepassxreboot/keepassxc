/*
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
 *  Copyright (C) 2017-2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_YUBIKEY_INTERFACE_USB_H
#define KEEPASSX_YUBIKEY_INTERFACE_USB_H

#include "thirdparty/ykcore/ykdef.h"

#include "YubiKeyInterface.h"

/**
 * Singleton class to manage the USB interface to hardware key(s)
 */
class YubiKeyInterfaceUSB : public YubiKeyInterface
{
    Q_OBJECT

public:
    static YubiKeyInterfaceUSB* instance();

    bool findValidKeys() override;

    YubiKey::ChallengeResult
    challenge(YubiKeySlot slot, const QByteArray& challenge, Botan::secure_vector<char>& response) override;
    bool testChallenge(YubiKeySlot slot, bool* wouldBlock) override;

private:
    explicit YubiKeyInterfaceUSB();
    ~YubiKeyInterfaceUSB();

    static YubiKeyInterfaceUSB* m_instance;

    YubiKey::ChallengeResult performChallenge(void* key,
                                              int slot,
                                              bool mayBlock,
                                              const QByteArray& challenge,
                                              Botan::secure_vector<char>& response) override;
    bool performTestChallenge(void* key, int slot, bool* wouldBlock) override;

    // This map provides display names for the various USB PIDs of the Yubikeys
    const QHash<int, QString> m_pid_names = {{YUBIKEY_PID, "YubiKey 1/2"},
                                             {NEO_OTP_PID, "YubiKey NEO - OTP only"},
                                             {NEO_OTP_CCID_PID, "YubiKey NEO - OTP and CCID"},
                                             {NEO_CCID_PID, "YubiKey NEO - CCID only"},
                                             {NEO_U2F_PID, "YubiKey NEO - U2F only"},
                                             {NEO_OTP_U2F_PID, "YubiKey NEO - OTP and U2F"},
                                             {NEO_U2F_CCID_PID, "YubiKey NEO - U2F and CCID"},
                                             {NEO_OTP_U2F_CCID_PID, "YubiKey NEO - OTP, U2F and CCID"},
                                             {YK4_OTP_PID, "YubiKey 4/5 - OTP only"},
                                             {YK4_U2F_PID, "YubiKey 4/5 - U2F only"},
                                             {YK4_OTP_U2F_PID, "YubiKey 4/5 - OTP and U2F"},
                                             {YK4_CCID_PID, "YubiKey 4/5 - CCID only"},
                                             {YK4_OTP_CCID_PID, "YubiKey 4/5 - OTP and CCID"},
                                             {YK4_U2F_CCID_PID, "YubiKey 4/5 - U2F and CCID"},
                                             {YK4_OTP_U2F_CCID_PID, "YubiKey 4/5 - OTP, U2F and CCID"},
                                             {PLUS_U2F_OTP_PID, "YubiKey plus - OTP+U2F"}};
};

#endif // KEEPASSX_YUBIKEY_INTERFACE_USB_H
