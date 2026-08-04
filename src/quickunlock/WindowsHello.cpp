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
    int g_promptFocusCount = 0;

    void queueSecurityPromptFocus(int delay = 500)
    {
        QTimer::singleShot(delay, [] {
            auto hWnd = ::FindWindowA("Credential Dialog Xaml Host", nullptr);
            if (hWnd) {
                ::SetForegroundWindow(hWnd);
            } else if (++g_promptFocusCount <= 3) {
                queueSecurityPromptFocus();
                return;
            }
            g_promptFocusCount = 0;
        });
    }

    bool deriveEncryptionKey(QByteArray& challenge, QByteArray& key, QString& error)
    {
        error.clear();
        auto challengeBuffer = CryptographicBuffer::CreateFromByteArray(
            array_view<uint8_t>(reinterpret_cast<uint8_t*>(challenge.data()), challenge.size()));

        return AsyncTask::runAndWaitForFuture([&] {
            try {
                // The first time this is used a key-pair will be generated using the common name
                auto result = KeyCredentialManager::RequestCreateAsync(s_winHelloKeyName,
                                                                       KeyCredentialCreationOption::FailIfExists)
                                  .get();

                if (result.Status() == KeyCredentialStatus::CredentialAlreadyExists) {
                    result = KeyCredentialManager::OpenAsync(s_winHelloKeyName).get();
                } else if (result.Status() != KeyCredentialStatus::Success) {
                    error = QObject::tr("Failed to create Windows Hello credential.");
                    return false;
                }

                const auto signature = result.Credential().RequestSignAsync(challengeBuffer).get();
                if (signature.Status() != KeyCredentialStatus::Success) {
                    if (signature.Status() != KeyCredentialStatus::UserCanceled) {
                        error = QObject::tr("Failed to sign challenge using Windows Hello.");
                    }
                    return false;
                }

                // Use the SHA-256 hash of the challenge signature as the encryption key
                const auto response = signature.Result();
                CryptoHash hasher(CryptoHash::Sha256);
                hasher.addData({reinterpret_cast<const char*>(response.data()), static_cast<int>(response.Length())});
                key = hasher.result();
                return true;
            } catch (winrt::hresult_error const& ex) {
                error = QString::fromStdString(winrt::to_string(ex.message()));
                return false;
            }
        });
    }
} // namespace

bool WindowsHello::isAvailable() const
{
    auto task = concurrency::create_task([] { return KeyCredentialManager::IsSupportedAsync().get(); });
    return task.get();
}

QString WindowsHello::errorString() const
{
    return m_error;
}

bool WindowsHello::setKey(const QUuid& dbUuid, const QByteArray& data)
{
    QByteArray encrypted;
    if (!protect(data, encrypted)) {
        return false;
    }
    m_encryptedKeys.insert(dbUuid, encrypted);
    return true;
}

bool WindowsHello::getKey(const QUuid& dbUuid, QByteArray& data)
{
    data.clear();
    if (!hasKey(dbUuid)) {
        m_error = QObject::tr("Failed to get Windows Hello credential.");
        return false;
    }

    return unprotect(m_encryptedKeys.value(dbUuid), data);
}

bool WindowsHello::protect(const QByteArray& data, QByteArray& protectedData)
{
    protectedData.clear();
    queueSecurityPromptFocus();

    const auto ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
    auto challenge = Random::instance()->randomArray(ivSize);
    QByteArray key;
    if (!deriveEncryptionKey(challenge, key, m_error)) {
        return false;
    }

    SymmetricCipher cipher;
    const auto initialized = cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Encrypt, key, challenge);
    key.fill('\0');
    if (!initialized) {
        m_error = QObject::tr("Failed to init KeePassXC crypto.");
        return false;
    }
    protectedData = data;
    if (!cipher.finish(protectedData)) {
        protectedData.clear();
        m_error = QObject::tr("Failed to encrypt key data.");
        return false;
    }
    protectedData.prepend(challenge);
    return true;
}

bool WindowsHello::unprotect(const QByteArray& protectedData, QByteArray& data)
{
    data.clear();
    const auto ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
    if (protectedData.size() <= ivSize) {
        m_error = QObject::tr("Invalid Windows Hello key data.");
        return false;
    }

    queueSecurityPromptFocus();
    auto challenge = protectedData.left(ivSize);
    auto encrypted = protectedData.mid(ivSize);
    QByteArray key;
    if (!deriveEncryptionKey(challenge, key, m_error)) {
        return false;
    }

    SymmetricCipher cipher;
    const auto initialized = cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, challenge);
    key.fill('\0');
    if (!initialized) {
        m_error = QObject::tr("Failed to init KeePassXC crypto.");
        return false;
    }
    data = encrypted;
    if (!cipher.finish(data)) {
        data.clear();
        m_error = QObject::tr("Failed to decrypt key data.");
        return false;
    }
    return true;
}

void WindowsHello::reset(const QUuid& dbUuid)
{
    m_encryptedKeys.remove(dbUuid);
}

bool WindowsHello::hasKey(const QUuid& dbUuid) const
{
    return m_encryptedKeys.contains(dbUuid);
}

void WindowsHello::reset()
{
    m_encryptedKeys.clear();
}
