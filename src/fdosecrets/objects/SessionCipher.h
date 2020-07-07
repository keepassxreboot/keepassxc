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

#include "fdosecrets/GcryptMPI.h"
#include "fdosecrets/objects/Session.h"

class TestFdoSecrets;
class TestGuiFdoSecrets;

namespace FdoSecrets
{

    class CipherPair
    {
        Q_DISABLE_COPY(CipherPair)
    public:
        CipherPair() = default;
        virtual ~CipherPair() = default;
        virtual SecretStruct encrypt(const SecretStruct& input) = 0;
        virtual SecretStruct decrypt(const SecretStruct& input) = 0;
        virtual bool isValid() const = 0;
        virtual QVariant negotiationOutput() const = 0;
    };

    class PlainCipher : public CipherPair
    {
        Q_DISABLE_COPY(PlainCipher)
    public:
        static constexpr const char Algorithm[] = "plain";

        PlainCipher() = default;
        SecretStruct encrypt(const SecretStruct& input) override
        {
            return input;
        }

        SecretStruct decrypt(const SecretStruct& input) override
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
        bool m_valid;
        QByteArray m_privateKey;
        QByteArray m_publicKey;
        QByteArray m_aesKey;

        /**
         * Diffie Hullman Key Exchange
         * Given client public key, generate server private/public key pair and common secret.
         * This also sets m_publicKey to server's public key
         * @param clientPublicKey client public key
         * @param serverPrivate server private key
         * @param commonSecretBytes output common secret
         * @return true on success.
         */
        bool
        diffieHullman(const GcryptMPI& clientPublicKey, const GcryptMPI& serverPrivate, QByteArray& commonSecretBytes);

        /**
         * Perform HKDF defined in RFC5869, using sha256 as hash function
         * @param IKM input keying material
         * @return derived 128-bit key suitable for AES
         */
        QByteArray hkdf(const QByteArray& IKM);

        /**
         * Add PKCS#7 style padding to input inplace
         * @param input
         * @param blockSize the block size to use, must be 2's power
         * @return reference to input for chaining
         */
        QByteArray& padPkcs7(QByteArray& input, int blockSize);

        /**
         * Remove PKCS#7 style padding from input inplace
         * @param input
         * @return reference to input for chaining
         */
        QByteArray& unpadPkcs7(QByteArray& input);

        bool initialize(GcryptMPI clientPublic, GcryptMPI serverPublic, GcryptMPI serverPrivate);

        DhIetf1024Sha256Aes128CbcPkcs7()
            : m_valid(false)
        {
        }

    public:
        static constexpr const char Algorithm[] = "dh-ietf1024-sha256-aes128-cbc-pkcs7";

        explicit DhIetf1024Sha256Aes128CbcPkcs7(const QByteArray& clientPublicKeyBytes);

        SecretStruct encrypt(const SecretStruct& input) override;

        SecretStruct decrypt(const SecretStruct& input) override;

        bool isValid() const override;

        QVariant negotiationOutput() const override;

    private:
        /**
         * For test only, fix the server side private and public key.
         */
        static void fixNextServerKeys(GcryptMPI priv, GcryptMPI pub);
        static GcryptMPI NextPrivKey;
        static GcryptMPI NextPubKey;

    private:
        Q_DISABLE_COPY(DhIetf1024Sha256Aes128CbcPkcs7);
        friend class ::TestFdoSecrets;
        friend class ::TestGuiFdoSecrets;
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_SESSIONCIPHER_H
