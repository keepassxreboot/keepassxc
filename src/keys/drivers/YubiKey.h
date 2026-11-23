/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
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

#ifndef KEEPASSXC_YUBIKEY_H
#define KEEPASSXC_YUBIKEY_H

#include <functional>

#include <QHash>
#include <QMultiMap>
#include <QMutex>
#include <QObject>
#include <QRecursiveMutex>
#include <QTimer>

#include <botan/secmem.h>

typedef QPair<unsigned int, int> YubiKeySlot;
Q_DECLARE_METATYPE(YubiKeySlot);

/**
 * Singleton class to manage the interface to hardware key(s)
 */
class YubiKey : public QObject
{
    Q_OBJECT

public:
    using KeyMap = QMultiMap<YubiKeySlot, QString>;

    enum class ChallengeResult : int
    {
        YCR_ERROR = 0,
        YCR_SUCCESS = 1,
        YCR_WOULDBLOCK = 2,
        YCR_KEYNOTFOUND = 3,
        YCR_AUTHFAILED = 4, // TPM2 PIN/password was wrong
    };

    /**
     * TPM2 keys reuse YubiKeySlot: first = persistent TPM handle (0x81xxxxxx),
     * second = TPM_SLOT_FLAG | mode. Slots 1 and 2 remain real YubiKey slots.
     */
    static constexpr int TPM_SLOT_FLAG = 0x100;
    static constexpr int TPM_MODE_FIXED64 = 1;
    static constexpr int TPM_MODE_VARIABLE = 2; // Yubico HMAC_LT64
    static constexpr int TPM_PIN_FLAG = 0x200; // key is protected by a PIN/password (authValue)
    static constexpr int TPM_MODE_MASK = 0xFF;
    static bool isTpmSlot(const YubiKeySlot& slot)
    {
        return (slot.second & TPM_SLOT_FLAG) != 0;
    }
    static bool tpmSlotNeedsPin(const YubiKeySlot& slot)
    {
        return isTpmSlot(slot) && (slot.second & TPM_PIN_FLAG) != 0;
    }
    static int tpmMode(const YubiKeySlot& slot)
    {
        return slot.second & TPM_MODE_MASK;
    }
    static bool isValidTpmSlotValue(int slot)
    {
        const int mode = slot & TPM_MODE_MASK;
        return (slot & TPM_SLOT_FLAG) && (slot & ~(TPM_SLOT_FLAG | TPM_PIN_FLAG | TPM_MODE_MASK)) == 0
               && (mode == TPM_MODE_FIXED64 || mode == TPM_MODE_VARIABLE);
    }

    /**
     * Callback used to ask the user for a TPM2 PIN/password. Installed by the GUI or CLI.
     * @param slot key that needs the PIN
     * @param message text to show with the prompt (previous failure, lockout warning); empty if none
     * @param pin receives the PIN (UTF-8)
     * @param remember receives whether the PIN may be cached until the database is locked/closed
     * @return false if the user cancelled
     */
    using PinProvider = std::function<
        bool(const YubiKeySlot& slot, const QString& message, Botan::secure_vector<char>& pin, bool& remember)>;
    static void setPinProvider(PinProvider provider);

    /**
     * Encode a PIN as UTF-8 directly into secure (locked, zeroizing) memory, without an
     * intermediate QByteArray.
     */
    static Botan::secure_vector<char> pinFromString(const QString& text);
    static bool
    requestPin(const YubiKeySlot& slot, const QString& message, Botan::secure_vector<char>& pin, bool& remember);

    /**
     * @return a warning about TPM lockout state to show before asking for the PIN, or an empty string
     */
    QString pinNotice(const YubiKeySlot& slot);

    static YubiKey* instance();
    bool isInitialized();

    bool findValidKeys();
    void findValidKeysAsync();

    KeyMap foundKeys();

    /**
     * @return after a key scan: why TPM2 keys that were found cannot be used (e.g. keys bound to a PCR policy,
     * which is not supported), or an empty string
     */
    QString unusableTpmKeysHint() const;
    int connectedKeys();

    ChallengeResult challenge(YubiKeySlot slot,
                              const QByteArray& challenge,
                              Botan::secure_vector<char>& response,
                              const Botan::secure_vector<char>* pin = nullptr);
    bool testChallenge(YubiKeySlot slot,
                       bool* wouldBlock = nullptr,
                       const Botan::secure_vector<char>* pin = nullptr,
                       ChallengeResult* result = nullptr);

    QString errorMessage();

signals:
    /**
     * Emitted when a detection process completes. Use the `detectedSlots`
     * accessor function to get information on the available slots.
     *
     * @param found - true if a key was found
     */
    void detectComplete(bool found);

    /**
     * Emitted when user needs to interact with the hardware key to continue
     */
    void userInteractionRequest();

    /**
     * Emitted before/after a challenge-response is performed
     */
    void challengeStarted();
    void challengeCompleted();

private:
    explicit YubiKey();
    bool tpmEnabled() const;

    static YubiKey* m_instance;

    QTimer m_interactionTimer;
    bool m_initialized = false;
    bool m_findingKeys = false;
    QString m_error;

    // Prevents multiple simultaneous operations on hardware keys
    static QMutex s_interfaceMutex;

    KeyMap m_usbKeys;
    KeyMap m_pcscKeys;
    KeyMap m_tpmKeys;
    QString m_tpmKeysHint;

    int m_connectedKeys = 0;

    Q_DISABLE_COPY(YubiKey)
};

#endif // KEEPASSXC_YUBIKEY_H
