/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#include "WindowsHello.h"

#include <Userconsentverifierinterop.h>
#include <winrt/base.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.security.credentials.h>
#include <winrt/windows.security.cryptography.h>
#include <winrt/windows.storage.streams.h>

#include "core/AsyncTask.h"
#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"

#include <QTimer>
#include <QWindow>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Security::Credentials;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage::Streams;

namespace
{
    const std::wstring s_winHelloKeyName{L"keepassxc_winhello"};

    void queueSecurityPromptFocus(int delay = 500)
    {
        QTimer::singleShot(delay, [] {
            auto hWnd = ::FindWindowA("Credential Dialog Xaml Host", nullptr);
            if (hWnd) {
                ::SetForegroundWindow(hWnd);
            }
        });
    }

    bool deriveEncryptionKey(QByteArray& challenge, QByteArray& key, QString& error)
    {
        error.clear();
        auto challengeBuffer = CryptographicBuffer::CreateFromByteArray(
            array_view<uint8_t>(reinterpret_cast<uint8_t*>(challenge.data()), challenge.size()));

        return AsyncTask::runAndWaitForFuture([&] {
            // The first time this is used a key-pair will be generated using the common name
            auto result =
                KeyCredentialManager::RequestCreateAsync(s_winHelloKeyName, KeyCredentialCreationOption::FailIfExists)
                    .get();

            if (result.Status() == KeyCredentialStatus::CredentialAlreadyExists) {
                result = KeyCredentialManager::OpenAsync(s_winHelloKeyName).get();
            } else if (result.Status() != KeyCredentialStatus::Success) {
                error = QObject::tr("Failed to create Windows Hello credential.");
                return false;
            }

            const auto signature = result.Credential().RequestSignAsync(challengeBuffer).get();
            if (signature.Status() != KeyCredentialStatus::Success) {
                error = QObject::tr("Failed to sign challenge using Windows Hello.");
                return false;
            }

            // Use the SHA-256 hash of the challenge signature as the encryption key
            const auto response = signature.Result();
            CryptoHash hasher(CryptoHash::Sha256);
            hasher.addData({reinterpret_cast<const char*>(response.data()), static_cast<int>(response.Length())});
            key = hasher.result();
            return true;
        });
    }
} // namespace

WindowsHello* WindowsHello::m_instance{nullptr};
WindowsHello* WindowsHello::instance()
{
    if (!m_instance) {
        m_instance = new WindowsHello();
    }
    return m_instance;
}

WindowsHello::WindowsHello(QObject* parent)
    : QObject(parent)
{
    concurrency::create_task([this] {
        bool state = KeyCredentialManager::IsSupportedAsync().get();
        m_available = state;
        emit availableChanged(m_available);
    });
}

bool WindowsHello::isAvailable() const
{
    return m_available;
}

QString WindowsHello::errorString() const
{
    return m_error;
}

bool WindowsHello::storeKey(const QString& dbPath, const QByteArray& data)
{
    queueSecurityPromptFocus();

    // Generate a random challenge that will be signed by Windows Hello
    // to create the key. The challenge is also used as the IV.
    auto ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
    auto challenge = Random::instance()->randomArray(ivSize);
    QByteArray key;
    if (!deriveEncryptionKey(challenge, key, m_error)) {
        return false;
    }

    // Encrypt the data using AES-256-CBC
    SymmetricCipher cipher;
    if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Encrypt, key, challenge)) {
        m_error = tr("Failed to init KeePassXC crypto.");
        return false;
    }
    QByteArray encrypted = data;
    if (!cipher.finish(encrypted)) {
        m_error = tr("Failed to encrypt key data.");
        return false;
    }

    // Prepend the challenge/IV to the encrypted data
    encrypted.prepend(challenge);
    m_encryptedKeys.insert(dbPath, encrypted);
    return true;
}

bool WindowsHello::getKey(const QString& dbPath, QByteArray& data)
{
    data.clear();
    if (!hasKey(dbPath)) {
        m_error = tr("Failed to get Windows Hello credential.");
        return false;
    }

    queueSecurityPromptFocus();

    // Read the previously used challenge and encrypted data
    auto ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
    const auto& keydata = m_encryptedKeys.value(dbPath);
    auto challenge = keydata.left(ivSize);
    auto encrypted = keydata.mid(ivSize);
    QByteArray key;

    if (!deriveEncryptionKey(challenge, key, m_error)) {
        return false;
    }

    // Decrypt the data using the generated key and IV from above
    SymmetricCipher cipher;
    if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, challenge)) {
        m_error = tr("Failed to init KeePassXC crypto.");
        return false;
    }

    // Store the decrypted data into the passed parameter
    data = encrypted;
    if (!cipher.finish(data)) {
        data.clear();
        m_error = tr("Failed to decrypt key data.");
        return false;
    }

    return true;
}

void WindowsHello::reset(const QString& dbPath)
{
    m_encryptedKeys.remove(dbPath);
}

bool WindowsHello::hasKey(const QString& dbPath) const
{
    return m_encryptedKeys.contains(dbPath);
}

void WindowsHello::reset()
{
    m_encryptedKeys.clear();
}
