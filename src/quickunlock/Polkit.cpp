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

#include "Polkit.h"

#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"
#include "gui/osutils/nixutils/NixUtils.h"

#include <QDebug>
#include <QtDBus>

#include <botan/mem_ops.h>
#include <cerrno>

extern "C" {
#include <keyutils.h>
}

const QString polkit_service = "org.freedesktop.PolicyKit1";
const QString polkit_object = "/org/freedesktop/PolicyKit1/Authority";

Polkit::Polkit()
{
    PolkitSubject::registerMetaType();
    PolkitAuthorizationResults::registerMetaType();
    PolkitActionDescription::registerMetaType();

    /* Note we explicitly use our own dbus path here, as the ::systemBus() method could be overridden
       through an environment variable to return an alternative bus path. This bus could have an application
       pretending to be polkit running on it, which could approve every authentication request

       Most Linux distros place the system bus at this exact path, so it is hard-coded.
       For any other distros, this path will need to be patched before compilation.
    */
    QDBusConnection bus =
        QDBusConnection::connectToBus("unix:path=/run/dbus/system_bus_socket", "keepassxc_polkit_dbus");

    m_available = bus.isConnected();
    if (!m_available) {
        qWarning() << "polkit: Failed to connect to system dbus (this may be due to a non-standard dbus path)";
        return;
    }

    m_available = bus.interface()->isServiceRegistered(polkit_service);

    if (!m_available) {
        qWarning() << "polkit: Polkit is not registered on dbus";
        return;
    }

    // Initiate the Polkit dbus interface
    m_polkit.reset(new org::freedesktop::PolicyKit1::Authority(polkit_service, polkit_object, bus));

    // Reset available state and check Polkit registered actions for KeePassXC
    m_available = false;
    auto kpxcAction = QStringLiteral("org.keepassxc.KeePassXC.unlockDatabase");
    auto actions = m_polkit->EnumerateActions("");
    for (const auto& action : actions.value()) {
        if (action.actionId == kpxcAction) {
            m_available = true;
            break;
        }
    }

    if (!m_available) {
        qWarning() << "polkit: KeePassXC Polkit action is not installed";
    }
}

Polkit::~Polkit()
{
}

void Polkit::reset(const QUuid& dbUuid)
{
    m_sessionKeys.remove(dbUuid);
    nixUtils()->removeSecret(dbUuid.toString());
}

bool Polkit::isAvailable() const
{
    return m_available;
}

void Polkit::reset()
{
    m_sessionKeys.clear();
    nixUtils()->removeAllSecrets();
}

bool Polkit::setKey(const QUuid& dbUuid, const QByteArray& data)
{
    reset(dbUuid);

    // Prompt for a pin to use as session key
    QByteArray key;
    if (!promptPin(0, key)) {
        return false;
    }

    auto iv = randomGen()->randomArray(SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM));

    SymmetricCipher aes256Encrypt;
    if (!aes256Encrypt.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Encrypt, key, iv)) {
        m_error = QObject::tr("Failed to init KeePassXC crypto.");
        return false;
    }

    // Encrypt the database key
    QByteArray encrypted = data;
    if (!aes256Encrypt.finish(encrypted)) {
        m_error = QObject::tr("Failed to encrypt key data.");
        return false;
    }

    // Store the session key and save the encrypted master key to the keyring
    m_sessionKeys.insert(dbUuid, key);
    nixUtils()->saveSecret(dbUuid.toString(), encrypted.prepend(iv));

    return true;
}

bool Polkit::getKey(const QUuid& dbUuid, QByteArray& data)
{
    if (!m_available || !hasKey(dbUuid)) {
        m_error = QObject::tr("No key is stored for this database.");
        return false;
    }

    QByteArray key;
    for (int pinAttempts = 1; pinAttempts <= MAX_PIN_ATTEMPTS; ++pinAttempts) {
        if (!m_sessionKeys.contains(dbUuid)) {
            // Request pin to obtain a session key
            if (!promptPin(pinAttempts, key)) {
                m_error = QObject::tr("Failed to obtain session key.");
                return false;
            }
        } else {
            // We already have the session key, prompt using polkit to authorize use
            if (!promptPolkit()) {
                // Error set in promptPolkit call
                return false;
            }
            key = m_sessionKeys.value(dbUuid);
        }

        // Retrieve the encrypted master key from the OS secret store
        QByteArray encData;
        if (!nixUtils()->getSecret(dbUuid.toString(), encData)) {
            m_error = QObject::tr("Failed to get credentials for quick unlock.");
            return false;
        }

        const auto& ivSize = SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM);
        const auto& iv = encData.left(ivSize);

        // Decrypt the data using the generated key and IV from above
        SymmetricCipher cipher;
        if (!cipher.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, iv)) {
            m_error = QObject::tr("Failed to init KeePassXC crypto.");
            return false;
        }

        // Attempt to decrypt the key data
        data = encData.mid(ivSize);
        if (cipher.finish(data)) {
            // Decryption succeeded, store the session key used
            m_sessionKeys.insert(dbUuid, key);
            return true;
        }
    }

    m_error = QObject::tr("Too many pin attempts.");
    return false;
}

bool Polkit::promptPolkit()
{
    PolkitSubject subject;
    subject.kind = "unix-process";
    subject.details.insert("pid", static_cast<uint>(QCoreApplication::applicationPid()));
    subject.details.insert("start-time", nixUtils()->getProcessStartTime());

    QMap<QString, QString> details;

    auto result = m_polkit->CheckAuthorization(
        subject,
        "org.keepassxc.KeePassXC.unlockDatabase",
        details,
        0x00000001,
        // AllowUserInteraction - wait for user to authenticate
        // https://www.freedesktop.org/software/polkit/docs/0.105/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html#eggdbus-enum-CheckAuthorizationFlags
        "");

    // A general error occurred
    if (result.isError()) {
        auto msg = result.error().message();
        m_error = QObject::tr("Polkit returned an error: %1").arg(msg);
        return false;
    }

    PolkitAuthorizationResults authResult = result.value();
    if (authResult.is_authorized) {
        return true;
    }

    // Failed to authenticate
    if (authResult.is_challenge) {
        m_error = QObject::tr("No Polkit authentication agent was available.");
    } else {
        m_error = QObject::tr("Polkit authorization failed.");
    }
    return false;
}

bool Polkit::hasKey(const QUuid& dbUuid) const
{
    // Check if the OS has a secret stored for this database UUID
    QByteArray tmp;
    return nixUtils()->getSecret(dbUuid.toString(), tmp);
}
