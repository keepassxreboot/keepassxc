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

#include "quickunlock/TouchID.h"
#include "gui/osutils/OSUtils.h"

/**
 * Store the serialized database key into the macOS key store. The OS handles encrypt/decrypt operations.
 * https://developer.apple.com/documentation/security/keychain_services/keychain_items
 */
bool TouchID::setKey(const QUuid& dbUuid, const QByteArray& key)
{
    if (key.isEmpty()) {
        qWarning("TouchID::setKey - provided key is empty");
        return false;
    }

    return osUtils->saveSecret(dbUuid.toString(), key);
}

/**
 * Retrieve serialized key data from the macOS Keychain after successful authentication
 * with TouchID or Watch interface.
 */
bool TouchID::getKey(const QUuid& dbUuid, QByteArray& key)
{
    key.clear();

    if (!hasKey(dbUuid)) {
        qWarning("TouchID::getKey - No stored key found");
        return false;
    }

    return osUtils->getSecret(dbUuid.toString(), key);
}

bool TouchID::hasKey(const QUuid& dbUuid) const
{
    QByteArray tmp;
    return osUtils->getSecret(dbUuid.toString(), tmp);
}

bool TouchID::isAvailable() const
{
    return macUtils()->isAuthPolicyAvailable(MacUtils::AuthPolicy::TouchId)
           || macUtils()->isAuthPolicyAvailable(MacUtils::AuthPolicy::Watch)
           || macUtils()->isAuthPolicyAvailable(MacUtils::AuthPolicy::PasswordFallback);
}

void TouchID::reset(const QUuid& dbUuid)
{
    osUtils->removeSecret(dbUuid.toString());
}

void TouchID::reset()
{
    osUtils->removeAllSecrets();
}
