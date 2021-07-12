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

#ifndef KEEPASSXC_FDOSECRETS_SESSION_H
#define KEEPASSXC_FDOSECRETS_SESSION_H

#include "fdosecrets/dbus/DBusObject.h"

namespace FdoSecrets
{
    class CipherPair;
    class Session : public DBusObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", DBUS_INTERFACE_SECRET_SESSION_LITERAL)

        explicit Session(QSharedPointer<CipherPair> cipher, const QString& peer, Service* parent);

    public:
        /**
         * @brief Create a new instance of `Session`.
         * @param cipher the negotiated cipher
         * @param peer connecting peer
         * @param parent the owning Service
         * @return pointer to newly created Session, or nullptr if error
         * This may be caused by
         *   - DBus path registration error
         */
        static Session* Create(QSharedPointer<CipherPair> cipher, const QString& peer, Service* parent);

        Q_INVOKABLE DBusResult close();

        /**
         * Encode the secret struct. Note only the value field is encoded.
         * @param input
         * @return
         */
        Secret encode(const Secret& input) const;

        /**
         * Decode the secret struct.
         * @param input
         * @return
         */
        Secret decode(const Secret& input) const;

        /**
         * The peer application that opened this session
         * @return
         */
        QString peer() const;

        QString id() const;

        Service* service() const;

    signals:
        /**
         * The session is going to be closed
         * @param sess
         */
        void aboutToClose();

    private:
        QSharedPointer<CipherPair> m_cipher;
        QString m_peer;
        QUuid m_id;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SESSION_H
