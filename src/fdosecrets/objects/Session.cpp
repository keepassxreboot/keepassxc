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

#include "fdosecrets/objects/SessionCipher.h"

#include "core/Tools.h"

namespace FdoSecrets
{

    QHash<QString, QVariant> Session::negoniationState;

    Session::Session(std::unique_ptr<CipherPair>&& cipher, const QString& peer, Service* parent)
        : DBusObject(parent)
        , m_cipher(std::move(cipher))
        , m_peer(peer)
        , m_id(QUuid::createUuid())
    {
        registerWithPath(QStringLiteral(DBUS_PATH_TEMPLATE_SESSION).arg(p()->objectPath().path(), id()),
                         new SessionAdaptor(this));
    }

    void Session::CleanupNegotiation(const QString& peer)
    {
        negoniationState.remove(peer);
    }

    DBusReturn<void> Session::close()
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

    std::unique_ptr<CipherPair> Session::CreateCiphers(const QString& peer,
                                                       const QString& algorithm,
                                                       const QVariant& input,
                                                       QVariant& output,
                                                       bool& incomplete)
    {
        Q_UNUSED(peer);
        incomplete = false;

        std::unique_ptr<CipherPair> cipher{};
        if (algorithm == QLatin1Literal("plain")) {
            cipher.reset(new PlainCipher);
        } else if (algorithm == QLatin1Literal("dh-ietf1024-sha256-aes128-cbc-pkcs7")) {
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

    SecretStruct Session::encode(const SecretStruct& input) const
    {
        auto output = m_cipher->encrypt(input);
        output.session = objectPath();
        return output;
    }

    SecretStruct Session::decode(const SecretStruct& input) const
    {
        return m_cipher->decrypt(input);
    }

} // namespace FdoSecrets
