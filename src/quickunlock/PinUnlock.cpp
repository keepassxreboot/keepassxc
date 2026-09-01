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

#include "PinUnlock.h"

#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "gui/osutils/OSUtils.h"

#include <QInputDialog>
#include <QRegularExpression>

const int PinUnlock::MIN_PIN_LENGTH = 6;
const int PinUnlock::MAX_PIN_LENGTH = 10;
const int PinUnlock::MAX_PIN_ATTEMPTS = 3;

bool PinUnlock::isAvailable() const
{
    return true;
}

bool PinUnlock::promptPin(int attempt, QByteArray& sessionKey)
{
    QString pin;

    if (attempt == 0) {
        // Loop until a valid pin has been entered or canceled
        QRegularExpression pinRegex("^\\d+$");
        while (true) {
            bool ok = false;
            pin = QInputDialog::getText(
                nullptr,
                QObject::tr("Quick Unlock Pin Entry"),
                QObject::tr("Enter a %1–%2 digit pin to use for quick unlock:").arg(MIN_PIN_LENGTH).arg(MAX_PIN_LENGTH),
                QLineEdit::Password,
                {},
                &ok);

            if (!ok) {
                m_error = QObject::tr("Pin setup was canceled. Quick unlock has not been enabled.");
                return false;
            }

            // Validate pin criteria
            if (pin.length() >= MIN_PIN_LENGTH && pin.length() <= MAX_PIN_LENGTH && pinRegex.match(pin).hasMatch()) {
                // Pin is valid, move to hashing
                break;
            }
        }
    } else {
        bool ok = false;
        pin = QInputDialog::getText(
            nullptr,
            QObject::tr("Quick Unlock Pin Entry"),
            QObject::tr("Enter quick unlock pin (%1 of %2 attempts):").arg(attempt).arg(MAX_PIN_ATTEMPTS),
            QLineEdit::Password,
            {},
            &ok);

        if (!ok) {
            // User canceled the pin entry dialog, record pin attempts
            m_error = QObject::tr("Pin entry was canceled.");
            return false;
        }
    }

    // Hash the pin then run it through Argon2 to derive the encryption key
    sessionKey.fill('\0', 32);
    Argon2Kdf kdf(Argon2Kdf::Type::Argon2id);
    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(pin.toLatin1());
    if (!kdf.transform(hash.result(), sessionKey)) {
        m_error = QObject::tr("Failed to derive key using Argon2");
        return false;
    }

    return true;
}

bool PinUnlock::setKey(const QUuid& dbUuid, const QByteArray& data)
{
    QByteArray key;
    if (!promptPin(0, key)) {
        // Pin entry was canceled or failed, error set by promptPin
        return false;
    }

    // Generate a random IV
    const auto iv = Random::instance()->randomArray(SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM));

    // Encrypt the data using AES-256-GCM
    SymmetricCipher cipher;
    if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Encrypt, key, iv)) {
        m_error = QObject::tr("Failed to init KeePassXC crypto.");
        return false;
    }
    QByteArray encrypted = data;
    if (!cipher.finish(encrypted)) {
        m_error = QObject::tr("Failed to encrypt key data.");
        return false;
    }

    // Store the encrypted data
    saveKey(dbUuid, encrypted.prepend(iv));
    return true;
}

bool PinUnlock::getKey(const QUuid& dbUuid, QByteArray& data)
{
    data.clear();
    bool hasSecret = m_encryptedKeys.contains(dbUuid);
    if (!hasSecret) {
        // Check if the OS has a secret stored for this database UUID
        QByteArray tmp;
        if (osUtils->getSecret(dbUuid.toString(), tmp)) {
            // Cache the secret in memory
            m_encryptedKeys.insert(dbUuid, qMakePair(1, tmp));
        } else {
            m_error = QObject::tr("Failed to get credentials for quick unlock.");
            return false;
        }
    }

    // Restrict pin attempts per database
    const auto& pairData = m_encryptedKeys.value(dbUuid);
    for (int pinAttempts = pairData.first; pinAttempts <= MAX_PIN_ATTEMPTS; ++pinAttempts) {
        QByteArray key;
        if (!promptPin(pinAttempts, key)) {
            // Pin entry was canceled or failed, error set by promptPin
            m_encryptedKeys.insert(dbUuid, qMakePair(pinAttempts, pairData.second));
            return false;
        }

        // Read the previously used challenge and encrypted data
        const auto& ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
        const auto& iv = pairData.second.left(ivSize);

        // Decrypt the data using the generated key and IV from above
        SymmetricCipher cipher;
        if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, iv)) {
            m_error = QObject::tr("Failed to init KeePassXC crypto.");
            return false;
        }

        // Attempt to decrypt the key data
        data = pairData.second.mid(ivSize);
        if (cipher.finish(data)) {
            // Decryption succeeded, reset the pin attempts
            m_encryptedKeys.insert(dbUuid, qMakePair(1, pairData.second));
            return true;
        }
    }

    data.clear();
    m_error = QObject::tr("Too many pin attempts.");
    reset(dbUuid);
    return false;
}

void PinUnlock::saveKey(const QUuid& dbUuid, const QByteArray& data)
{
    // Save the key to the OS secret store
    if (!osUtils->saveSecret(dbUuid.toString(), data)) {
        qWarning("PinUnlock - Failed to save quick unlock credentials.");
    }
    // Store the encrypted key in memory
    m_encryptedKeys.insert(dbUuid, qMakePair(1, data));
}

bool PinUnlock::hasKey(const QUuid& dbUuid) const
{
    bool hasSecret = m_encryptedKeys.contains(dbUuid);
    if (!hasSecret) {
        // Check if the OS has a secret stored for this database UUID
        QByteArray tmp;
        hasSecret = osUtils->getSecret(dbUuid.toString(), tmp);
    }
    return hasSecret;
}

void PinUnlock::reset(const QUuid& dbUuid)
{
    m_encryptedKeys.remove(dbUuid);
    osUtils->removeSecret(dbUuid.toString());
}

void PinUnlock::reset()
{
    m_encryptedKeys.clear();
    osUtils->removeAllSecrets();
}
