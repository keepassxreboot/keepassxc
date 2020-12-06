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

#include "fdosecrets/dbus/DBusMgr.h"
#include "fdosecrets/objects/SessionCipher.h"

namespace FdoSecrets
{
    DBusClient::DBusClient(DBusMgr& dbus, const QString& address, uint pid, const QString& name)
        : m_dbus(dbus)
        , m_address(address)
        , m_pid(pid)
        , m_name(name)
    {
    }

    bool DBusClient::itemAuthorized(const QUuid& uuid) const
    {
        return m_authorizedAll || m_authorizedEntries.find(uuid) != m_authorizedEntries.end();

    }

    void DBusClient::setItemAuthorized(const QUuid& uuid)
    {
        m_authorizedEntries.insert(uuid);
    }

    void DBusClient::setAllAuthorized(bool authorized)
    {
        m_authorizedAll = authorized;
    }

    void DBusClient::disconnectDBus()
    {
        // clear authorization
        m_authorizedAll = false;
        m_authorizedEntries.clear();
        // notify DBusMgr about the removal
        m_dbus.removeClient(this);
    }

    QSharedPointer<CipherPair> DBusClient::negotiateCipher(const QString& algorithm, const QVariant& input, QVariant& output, bool& incomplete)
    {
        incomplete = false;

        QSharedPointer<CipherPair> cipher{};
        if (algorithm == QLatin1String(PlainCipher::Algorithm)) {
            cipher.reset(new PlainCipher);
        } else if (algorithm == QLatin1String(DhIetf1024Sha256Aes128CbcPkcs7::Algorithm)) {
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
