/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "Session.h"

#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/SessionCipher.h"

#include "core/Tools.h"

namespace FdoSecrets
{
    Session* Session::Create(QSharedPointer<CipherPair> cipher, const QString& peer, Service* parent)
    {
        QScopedPointer<Session> res{new Session(std::move(cipher), peer, parent)};
        if (!res->dbus()->registerObject(res.data())) {
            return nullptr;
        }

        return res.take();
    }

    Session::Session(QSharedPointer<CipherPair> cipher, const QString& peer, Service* parent)
        : DBusObject(parent)
        , m_cipher(std::move(cipher))
        , m_peer(peer)
        , m_id(QUuid::createUuid())
    {
    }

    DBusResult Session::close()
    {
        emit aboutToClose();
        deleteLater();

        return {};
    }

    QString Session::peer() const
    {
        return m_peer;
    }

    QString Session::id() const
    {
        return Tools::uuidToHex(m_id);
    }

    Service* Session::service() const
    {
        return qobject_cast<Service*>(parent());
    }

    Secret Session::encode(const Secret& input) const
    {
        auto output = m_cipher->encrypt(input);
        output.session = this;
        return output;
    }

    Secret Session::decode(const Secret& input) const
    {
        Q_ASSERT(input.session == this);
        return m_cipher->decrypt(input);
    }
} // namespace FdoSecrets
