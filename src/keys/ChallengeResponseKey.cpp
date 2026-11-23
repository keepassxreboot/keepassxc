/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "ChallengeResponseKey.h"

#include "core/AsyncTask.h"
#include "core/Config.h"

#include <botan/mem_ops.h>

namespace
{
    // The remembered PIN may only travel with the Quick Unlock data if the user opted in
    bool savePinWithQuickUnlock()
    {
        return config()->get(Config::Security_QuickUnlock).toBool()
               && config()->get(Config::Security_QuickUnlockTpm2Pin).toBool();
    }
} // namespace

QUuid ChallengeResponseKey::UUID("e092495c-e77d-498b-84a1-05ae0d955508");

ChallengeResponseKey::ChallengeResponseKey(YubiKeySlot keySlot)
    : Key(UUID)
    , m_keySlot(keySlot)
{
}

ChallengeResponseKey::~ChallengeResponseKey()
{
    clearCachedPin();
}

bool ChallengeResponseKey::needsPin() const
{
    return YubiKey::tpmSlotNeedsPin(m_keySlot);
}

void ChallengeResponseKey::setCachedPin(const Botan::secure_vector<char>& pin)
{
    // Wipe first: assigning into the existing buffer would leave the tail of a longer previous PIN behind
    clearCachedPin();
    m_pin = pin;
}

bool ChallengeResponseKey::quickUnlockDataOutdated() const
{
    return m_quickUnlockDataOutdated;
}

bool ChallengeResponseKey::hasCachedPin() const
{
    return !m_pin.empty();
}

void ChallengeResponseKey::clearCachedPin()
{
    // Botan's secure allocator also scrubs on deallocation, but wipe the whole buffer right away
    Botan::secure_scrub_memory(m_pin.data(), m_pin.capacity());
    m_pin.clear();
}

QByteArray ChallengeResponseKey::rawKey() const
{
    return {m_key.data(), static_cast<int>(m_key.size())};
}

void ChallengeResponseKey::setRawKey(const QByteArray&)
{
    // Nothing to do here
}

YubiKeySlot ChallengeResponseKey::slotData() const
{
    return m_keySlot;
}

QString ChallengeResponseKey::error() const
{
    return m_error;
}

bool ChallengeResponseKey::challenge(const QByteArray& challenge)
{
    m_error.clear();

    // No retry limit here: the TPM's own dictionary-attack protection locks out repeated failures, and every
    // failed attempt is reported to the user. The loop ends on success, cancellation or any error that is not a
    // wrong PIN. Invariant: an attempt is only ever repeated after asking the user for a PIN again. Without a PIN
    // (plain entry) a refused authorization is final, otherwise the same empty password would be sent over and
    // over until the TPM locks out.
    const bool usePin = needsPin();
    Botan::secure_vector<char> pin;
    bool remember = false;
    bool freshPin = false;
    QString message; // shown with the prompt: lockout warning first, later the previous failure
    auto result = YubiKey::ChallengeResult::YCR_ERROR;

    for (int attempt = 0;; ++attempt) {
        if (usePin) {
            if (!m_pin.empty()) {
                pin = m_pin;
                freshPin = false;
            } else {
                remember = false;
                if (attempt == 0) {
                    message = AsyncTask::runAndWaitForFuture([&] { return YubiKey::instance()->pinNotice(m_keySlot); });
                }
                if (!YubiKey::requestPin(m_keySlot, message, pin, remember)) {
                    m_key.clear();
                    m_error = QObject::tr("TPM2 PIN/password entry was cancelled.");
                    return false;
                }
                freshPin = true;
            }
        }

        result = AsyncTask::runAndWaitForFuture(
            [&] { return YubiKey::instance()->challenge(m_keySlot, challenge, m_key, usePin ? &pin : nullptr); });

        if (usePin && result == YubiKey::ChallengeResult::YCR_AUTHFAILED) {
            // Drop a cached PIN that no longer works and ask again, telling the user what the TPM did
            clearCachedPin();
            message = YubiKey::instance()->errorMessage();
            continue;
        }
        break;
    }

    if (result != YubiKey::ChallengeResult::YCR_SUCCESS) {
        // Record the error message
        m_key.clear();
        m_error = YubiKey::instance()->errorMessage();
        return false;
    }

    if (usePin && freshPin && remember) {
        setCachedPin(pin);
        // The PIN was only just remembered: Quick Unlock data stored earlier does not contain it yet
        m_quickUnlockDataOutdated = savePinWithQuickUnlock();
    }
    return true;
}

/**
 * Serialized form is used by Quick Unlock only. A remembered TPM2 PIN is appended
 * after the slot if the user enabled saving it with Quick Unlock.
 */
QByteArray ChallengeResponseKey::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << uuid().toRfc4122() << m_keySlot;
    if (needsPin() && !m_pin.empty() && savePinWithQuickUnlock()) {
        QByteArray pinBytes(m_pin.data(), static_cast<qsizetype>(m_pin.size()));
        stream << pinBytes;
        Botan::secure_scrub_memory(pinBytes.data(), static_cast<size_t>(pinBytes.size()));
    }
    return data;
}

void ChallengeResponseKey::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    QByteArray uuidData;
    stream >> uuidData;
    if (uuid().toRfc4122() == uuidData) {
        stream >> m_keySlot;

        // Optional trailing PIN (absent in data written without the option or by older versions)
        if (!stream.atEnd() && needsPin()) {
            QByteArray pin;
            stream >> pin;
            if (stream.status() == QDataStream::Ok && !pin.isEmpty()) {
                if (savePinWithQuickUnlock()) {
                    clearCachedPin();
                    m_pin.assign(pin.constData(), pin.constData() + pin.size());
                } else {
                    // The stored data keeps a PIN that the settings no longer allow: it should be stored again
                    m_quickUnlockDataOutdated = true;
                }
            }
            Botan::secure_scrub_memory(pin.data(), static_cast<size_t>(pin.size()));
        }
    }
}
