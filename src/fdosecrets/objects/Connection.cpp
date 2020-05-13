/*
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

#include "Connection.h"

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/Session.h"

namespace FdoSecrets
{

    Connection::Connection(Service* parent, const QString& peerName)
        : QObject(parent)
        , m_peerName(peerName)
    {
    }

    Connection::~Connection()
    {
        disconnectDBus();
        for (auto& sess : m_sessions) {
            delete sess;
        }
    }

    void Connection::addSession(Session* sess)
    {
        m_sessions.append(sess);
        connect(sess, &Session::aboutToClose, this, [this, sess]() { m_sessions.removeAll(sess); });
    }

    bool Connection::authorized(const QUuid& uuid) const
    {
        if (!m_connected) {
            return false;
        }

        if (!FdoSecrets::settings()->confirmAccessItem()) {
            return true;
        }

        return m_authorizedAll || m_authorizedEntries.contains(uuid);
    }

    void Connection::setAuthorizedAll()
    {
        m_authorizedAll = true;
    }

    void Connection::setAuthorizedItem(const QUuid& uuid)
    {
        m_authorizedEntries.insert(uuid);
    }

    void Connection::disconnectDBus()
    {
        if (m_connected) {
            m_connected = false;
            emit disconnectedDBus();
        }
    }

} // namespace FdoSecrets
