/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_SESSIONCIPHER_H
#define KEEPASSXC_FDOSECRETS_SESSIONCIPHER_H

#include "fdosecrets/dbus/DBusTypes.h"

#include <QSharedPointer>

namespace Botan
{
    class DH_PrivateKey;
}

namespace FdoSecrets
{
    class CipherPair
    {
        Q_DISABLE_COPY(CipherPair)
    public:
        CipherPair() = default;
        virtual ~CipherPair() = default;
        virtual Secret encrypt(const Secret& input) = 0;
        virtual Secret decrypt(const Secret& input) = 0;
        virtual bool isValid() const = 0;
        virtual QVariant negotiationOutput() const = 0;
    };

    class PlainCipher : public CipherPair
    {
        Q_DISABLE_COPY(PlainCipher)
    public:
        static constexpr const char Algorithm[] = "plain";

        PlainCipher() = default;
        Secret encrypt(const Secret& input) override
        {
            return input;
        }

        Secret decrypt(const Secret& input) override
        {
            return input;
        }

        bool isValid() const override
        {
            return true;
        }

        QVariant negotiationOutput() const override
        {
            return QStringLiteral("");
        }
    };

    class DhIetf1024Sha256Aes128CbcPkcs7 : public CipherPair
    {
    public:
        static constexpr const char Algorithm[] = "dh-ietf1024-sha256-aes128-cbc-pkcs7";

        explicit DhIetf1024Sha256Aes128CbcPkcs7(const QByteArray& clientPublicKey);

        Secret encrypt(const Secret& input) override;
        Secret decrypt(const Secret& input) override;
        bool isValid() const override;
        QVariant negotiationOutput() const override;

        bool updateClientPublicKey(const QByteArray& clientPublicKey);

    private:
        Q_DISABLE_COPY(DhIetf1024Sha256Aes128CbcPkcs7);

        bool m_valid = false;
        QSharedPointer<Botan::DH_PrivateKey> m_privateKey;
        QByteArray m_aesKey;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SESSIONCIPHER_H
