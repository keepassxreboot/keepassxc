/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_YUBIKEY_INTERFACE_TPM_H
#define KEEPASSXC_YUBIKEY_INTERFACE_TPM_H

#include "YubiKeyInterface.h"

/**
 * Singleton class providing HMAC-SHA1 challenge-response using a keyed-hash
 * object persisted in a TPM 2.0 (persistent handles 0x81xxxxxx).
 *
 * The key is addressed as YubiKeySlot{handle, YubiKey::TPM_SLOT_FLAG | mode}.
 * The TPM mimics the YubiKey HMAC-SHA1 modes so a secret that is shared between
 * a TPM and a YubiKey yields identical responses:
 *   - Fixed 64 byte: challenge is PKCS7-padded to 64 bytes, HMAC over all 64.
 *   - Variable length (HMAC_LT64): as above, but the device strips the trailing
 *     run of bytes equal to the last byte before computing the HMAC.
 *
 * Keys must have userWithAuth set and no authPolicy. Keys protected by a PIN/password (authValue)
 * are addressed with YubiKey::TPM_PIN_FLAG; the PIN is supplied per call and never stored here.
 * On non-Linux builds or without KPXC_FEATURE_TPM2 this class is inert.
 */
class YubiKeyInterfaceTPM : public YubiKeyInterface
{
    Q_OBJECT

public:
    static YubiKeyInterfaceTPM* instance();

    YubiKey::KeyMap findValidKeys(int& connectedKeys) override;

    YubiKey::ChallengeResult
    challenge(YubiKeySlot slot, const QByteArray& challenge, Botan::secure_vector<char>& response) override;
    bool testChallenge(YubiKeySlot slot, bool* wouldBlock) override;

    YubiKey::ChallengeResult challengeWithPin(YubiKeySlot slot,
                                              const QByteArray& challenge,
                                              Botan::secure_vector<char>& response,
                                              const Botan::secure_vector<char>* pin);
    bool testChallengeWithPin(YubiKeySlot slot,
                              bool* wouldBlock,
                              const Botan::secure_vector<char>* pin,
                              YubiKey::ChallengeResult* result = nullptr);

    /**
     * Describe the TPM's dictionary-attack lockout state if it is relevant before asking for a PIN
     * (failed attempts already recorded, or locked out). Empty when there is nothing to warn about.
     */
    QString pinNotice(YubiKeySlot slot);

    /**
     * After findValidKeys(): explains why TPM2 HMAC keys that were found cannot be used (for example keys bound to
     * an authorization policy such as PCR values, which is not supported). Empty if there is nothing to explain.
     */
    QString unusableKeysHint() const;

    // Exposed for unit testing: the exact message the TPM must HMAC for a given challenge/mode
    static QByteArray buildMessage(const QByteArray& challenge, int mode);

private:
    explicit YubiKeyInterfaceTPM();
    ~YubiKeyInterfaceTPM() override = default;

    static YubiKeyInterfaceTPM* m_instance;
    QString m_unusableKeysHint;

    YubiKey::ChallengeResult performChallenge(void* key,
                                              int slot,
                                              bool mayBlock,
                                              const QByteArray& challenge,
                                              Botan::secure_vector<char>& response) override;
    bool performTestChallenge(void* key, int slot, bool* wouldBlock) override;
};

#endif // KEEPASSXC_YUBIKEY_INTERFACE_TPM_H
