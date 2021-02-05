/*
 *  Copyright (C) 2020 Aetf <aetf@unlimitedcode.works>
 *  Copyright (C) 2020 Jan Klötzke <jan@kloetzke.net>
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

#include "DBusClient.h"

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/dbus/DBusMgr.h"
#include "fdosecrets/objects/SessionCipher.h"

namespace FdoSecrets
{
    DBusClient::DBusClient(DBusMgr* dbus, const QString& address, uint pid, const QString& name)
        : m_dbus(dbus)
        , m_address(address)
        , m_pid(pid)
        , m_name(name)
    {
    }

    bool DBusClient::itemKnown(const QUuid& uuid) const
    {
        return m_authorizedAll || m_allowed.contains(uuid) || m_allowedOnce.contains(uuid) || m_denied.contains(uuid)
               || m_deniedOnce.contains(uuid);
    }

    bool DBusClient::itemAuthorized(const QUuid& uuid) const
    {
        if (!FdoSecrets::settings()->confirmAccessItem()) {
            // everyone is authorized if this is not enabled
            return true;
        }
        if (m_authorizedAll) {
            // this client is trusted
            return true;
        }
        if (m_deniedOnce.contains(uuid) || m_denied.contains(uuid)) {
            // explicitly denied
            return false;
        }
        if (m_allowedOnce.contains(uuid) || m_allowed.contains(uuid)) {
            // explicitly allowed
            return true;
        }
        // haven't asked, not authorized by default
        return false;
    }

    bool DBusClient::itemAuthorizedResetOnce(const QUuid& uuid)
    {
        auto auth = itemAuthorized(uuid);
        m_deniedOnce.remove(uuid);
        m_allowedOnce.remove(uuid);
        return auth;
    }

    void DBusClient::setItemAuthorized(const QUuid& uuid, AuthDecision auth)
    {
        // uuid should only be in exactly one set at any time
        m_allowed.remove(uuid);
        m_allowedOnce.remove(uuid);
        m_denied.remove(uuid);
        m_deniedOnce.remove(uuid);
        switch (auth) {
        case AuthDecision::Allowed:
            m_allowed.insert(uuid);
            break;
        case AuthDecision::AllowedOnce:
            m_allowedOnce.insert(uuid);
            break;
        case AuthDecision::Denied:
            m_denied.insert(uuid);
            break;
        case AuthDecision::DeniedOnce:
            m_deniedOnce.insert(uuid);
            break;
        default:
            break;
        }
    }

    void DBusClient::setAllAuthorized(bool authorized)
    {
        m_authorizedAll = authorized;
    }

    void DBusClient::clearAuthorization()
    {
        m_authorizedAll = false;
        m_allowed.clear();
        m_allowedOnce.clear();
        m_denied.clear();
        m_deniedOnce.clear();
    }

    void DBusClient::disconnectDBus()
    {
        clearAuthorization();
        // notify DBusMgr about the removal
        m_dbus->removeClient(this);
    }

    QSharedPointer<CipherPair>
    DBusClient::negotiateCipher(const QString& algorithm, const QVariant& input, QVariant& output, bool& incomplete)
    {
        incomplete = false;

        QSharedPointer<CipherPair> cipher{};
        if (algorithm == PlainCipher::Algorithm) {
            cipher.reset(new PlainCipher);
        } else if (algorithm == DhIetf1024Sha256Aes128CbcPkcs7::Algorithm) {
            QByteArray clientPublicKey = input.toByteArray();
            cipher.reset(new DhIetf1024Sha256Aes128CbcPkcs7(clientPublicKey));
        } else {
            // error notSupported
        }

        if (!cipher) {
            return {};
        }

        if (!cipher->isValid()) {
            qWarning() << "FdoSecrets: Error creating cipher";
            return {};
        }

        output = cipher->negotiationOutput();
        return cipher;
    }
} // namespace FdoSecrets
