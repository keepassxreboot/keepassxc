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

#include "fdosecrets/objects/DBusObject.h"
#include "fdosecrets/objects/Service.h"
#include "fdosecrets/objects/SessionCipher.h"
#include "fdosecrets/objects/adaptors/SessionAdaptor.h"

#include <QByteArray>
#include <QHash>
#include <QUuid>
#include <QVariant>

#include <memory>

namespace FdoSecrets
{

    class CipherPair;
    class Session : public DBusObjectHelper<Session, SessionAdaptor>
    {
        Q_OBJECT

        explicit Session(std::unique_ptr<CipherPair>&& cipher, const QString& peer, Service* parent);

    public:
        static std::unique_ptr<CipherPair> CreateCiphers(const QString& peer,
                                                         const QString& algorithm,
                                                         const QVariant& input,
                                                         QVariant& output,
                                                         bool& incomplete);
        static void CleanupNegotiation(const QString& peer);

        /**
         * @brief Create a new instance of `Session`.
         * @param cipher the negotiated cipher
         * @param peer connecting peer
         * @param parent the owning Service
         * @return pointer to newly created Session, or nullptr if error
         * This may be caused by
         *   - DBus path registration error
         */
        static Session* Create(std::unique_ptr<CipherPair>&& cipher, const QString& peer, Service* parent);

        DBusReturn<void> close();

        /**
         * Encode the secret struct. Note only the value field is encoded.
         * @param input
         * @return
         */
        SecretStruct encode(const SecretStruct& input) const;

        /**
         * Decode the secret struct.
         * @param input
         * @return
         */
        SecretStruct decode(const SecretStruct& input) const;

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
        bool registerSelf();

    private:
        std::unique_ptr<CipherPair> m_cipher;
        QString m_peer;
        QUuid m_id;

        static QHash<QString, QVariant> negotiationState;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SESSION_H
