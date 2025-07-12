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

#include <QInputDialog>
#include <QRegularExpression>

namespace
{
    constexpr int MIN_PIN_LENGTH = 6;
    constexpr int MAX_PIN_LENGTH = 10;
    constexpr int MAX_PIN_ATTEMPTS = 3;
} // namespace

bool PinUnlock::isAvailable() const
{
    return true;
}

QString PinUnlock::errorString() const
{
    return m_error;
}

bool PinUnlock::setKey(const QUuid& dbUuid, const QByteArray& data)
{
    QString pin;
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
            break;
        }
    }

    // Hash the pin then run it through Argon2 to derive the encryption key
    QByteArray key(32, '\0');
    Argon2Kdf kdf(Argon2Kdf::Type::Argon2id);
    CryptoHash hash(CryptoHash::Sha256);
    hash.addData(pin.toLatin1());
    if (!kdf.transform(hash.result(), key)) {
        m_error = QObject::tr("Failed to derive key using Argon2");
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

    // Prepend the IV to the encrypted data
    encrypted.prepend(iv);
    // Store the encrypted data and pin attempts
    m_encryptedKeys.insert(dbUuid, qMakePair(1, encrypted));

    return true;
}

bool PinUnlock::getKey(const QUuid& dbUuid, QByteArray& data)
{
    data.clear();
    if (!hasKey(dbUuid)) {
        m_error = QObject::tr("Failed to get credentials for quick unlock.");
        return false;
    }

    const auto& pairData = m_encryptedKeys.value(dbUuid);

    // Restrict pin attempts per database
    for (int pinAttempts = pairData.first; pinAttempts <= MAX_PIN_ATTEMPTS; ++pinAttempts) {
        bool ok = false;
        auto pin = QInputDialog::getText(
            nullptr,
            QObject::tr("Quick Unlock Pin Entry"),
            QObject::tr("Enter quick unlock pin (%1 of %2 attempts):").arg(pinAttempts).arg(MAX_PIN_ATTEMPTS),
            QLineEdit::Password,
            {},
            &ok);

        if (!ok) {
            m_error = QObject::tr("Pin entry was canceled.");
            return false;
        }

        // Hash the pin then run it through Argon2 to derive the encryption key
        QByteArray key(32, '\0');
        Argon2Kdf kdf(Argon2Kdf::Type::Argon2id);
        CryptoHash hash(CryptoHash::Sha256);
        hash.addData(pin.toLatin1());
        if (!kdf.transform(hash.result(), key)) {
            m_error = QObject::tr("Failed to derive key using Argon2");
            return false;
        }

        // Read the previously used challenge and encrypted data
        const auto ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
        const auto& keydata = pairData.second;
        auto challenge = keydata.left(ivSize);
        auto encrypted = keydata.mid(ivSize);

        // Decrypt the data using the generated key and IV from above
        SymmetricCipher cipher;
        if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, challenge)) {
            m_error = QObject::tr("Failed to init KeePassXC crypto.");
            return false;
        }

        // Store the decrypted data into the passed parameter
        data = encrypted;
        if (cipher.finish(data)) {
            // Reset the pin attempts
            m_encryptedKeys.insert(dbUuid, qMakePair(1, keydata));
            return true;
        }
    }

    data.clear();
    m_error = QObject::tr("Too many pin attempts.");
    reset(dbUuid);
    return false;
}

bool PinUnlock::hasKey(const QUuid& dbUuid) const
{
    return m_encryptedKeys.contains(dbUuid);
}

void PinUnlock::reset(const QUuid& dbUuid)
{
    m_encryptedKeys.remove(dbUuid);
}

void PinUnlock::reset()
{
    m_encryptedKeys.clear();
}
