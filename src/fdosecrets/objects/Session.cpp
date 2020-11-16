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

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/objects/SessionCipher.h"

#include "core/Tools.h"

namespace FdoSecrets
{

    QHash<QString, QVariant> Session::negotiationState;

    Session* Session::Create(std::unique_ptr<CipherPair>&& cipher, const QString& peer, Service* parent)
    {
        QScopedPointer<Session> res{new Session(std::move(cipher), peer, parent)};

        if (!res->registerSelf()) {
            return nullptr;
        }

        return res.take();
    }

    Session::Session(std::unique_ptr<CipherPair>&& cipher, const QString& peer, Service* parent)
        : DBusObjectHelper(parent)
        , m_cipher(std::move(cipher))
        , m_peer(peer)
        , m_id(QUuid::createUuid())
    {
    }

    bool Session::registerSelf()
    {
        auto path = QStringLiteral(DBUS_PATH_TEMPLATE_SESSION).arg(p()->objectPath().path(), id());
        bool ok = registerWithPath(path);
        if (!ok) {
            service()->plugin()->emitError(tr("Failed to register session on DBus at path '%1'").arg(path));
        }
        return ok;
    }

    void Session::CleanupNegotiation(const QString& peer)
    {
        negotiationState.remove(peer);
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

    Service* Session::service() const
    {
        return qobject_cast<Service*>(parent());
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
