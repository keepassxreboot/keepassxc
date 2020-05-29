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

#include "SessionCipher.h"

#include "crypto/CryptoHash.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"

#include <gcrypt.h>

#include <memory>

namespace
{
    constexpr const auto IETF1024_SECOND_OAKLEY_GROUP_P_HEX = "FFFFFFFFFFFFFFFFC90FDAA22168C234"
                                                              "C4C6628B80DC1CD129024E088A67CC74"
                                                              "020BBEA63B139B22514A08798E3404DD"
                                                              "EF9519B3CD3A431B302B0A6DF25F1437"
                                                              "4FE1356D6D51C245E485B576625E7EC6"
                                                              "F44C42E9A637ED6B0BFF5CB6F406B7ED"
                                                              "EE386BFB5A899FA5AE9F24117C4B1FE6"
                                                              "49286651ECE65381FFFFFFFFFFFFFFFF";
    constexpr const size_t KEY_SIZE_BYTES = 128;
    constexpr const int AES_KEY_LEN = 16; // 128 bits

    const auto IETF1024_SECOND_OAKLEY_GROUP_P = MpiFromHex(IETF1024_SECOND_OAKLEY_GROUP_P_HEX, false);
} // namespace

namespace FdoSecrets
{
    // XXX: remove the redundant definitions once we are at C++17
    constexpr char PlainCipher::Algorithm[];
    constexpr char DhIetf1024Sha256Aes128CbcPkcs7::Algorithm[];

    DhIetf1024Sha256Aes128CbcPkcs7::DhIetf1024Sha256Aes128CbcPkcs7(const QByteArray& clientPublicKeyBytes)
        : m_valid(false)
    {
        // read client public key
        auto clientPub = MpiFromBytes(clientPublicKeyBytes, false);

        // generate server side private, 128 bytes
        GcryptMPI serverPrivate = nullptr;
        if (NextPrivKey) {
            serverPrivate = std::move(NextPrivKey);
        } else {
            serverPrivate.reset(gcry_mpi_snew(KEY_SIZE_BYTES * 8));
            gcry_mpi_randomize(serverPrivate.get(), KEY_SIZE_BYTES * 8, GCRY_STRONG_RANDOM);
        }

        // generate server side public key
        GcryptMPI serverPublic = nullptr;
        if (NextPubKey) {
            serverPublic = std::move(NextPubKey);
        } else {
            serverPublic.reset(gcry_mpi_snew(KEY_SIZE_BYTES * 8));
            // the generator of Second Oakley Group is 2
            gcry_mpi_powm(
                serverPublic.get(), GCRYMPI_CONST_TWO, serverPrivate.get(), IETF1024_SECOND_OAKLEY_GROUP_P.get());
        }

        initialize(std::move(clientPub), std::move(serverPublic), std::move(serverPrivate));
    }

    bool
    DhIetf1024Sha256Aes128CbcPkcs7::initialize(GcryptMPI clientPublic, GcryptMPI serverPublic, GcryptMPI serverPrivate)
    {
        QByteArray commonSecretBytes;
        if (!diffieHullman(clientPublic, serverPrivate, commonSecretBytes)) {
            return false;
        }

        m_privateKey = MpiToBytes(serverPrivate);
        m_publicKey = MpiToBytes(serverPublic);

        m_aesKey = hkdf(commonSecretBytes);

        m_valid = true;
        return true;
    }

    bool DhIetf1024Sha256Aes128CbcPkcs7::diffieHullman(const GcryptMPI& clientPub,
                                                       const GcryptMPI& serverPrivate,
                                                       QByteArray& commonSecretBytes)
    {
        if (!clientPub || !serverPrivate) {
            return false;
        }

        // calc common secret
        GcryptMPI commonSecret(gcry_mpi_snew(KEY_SIZE_BYTES * 8));
        gcry_mpi_powm(commonSecret.get(), clientPub.get(), serverPrivate.get(), IETF1024_SECOND_OAKLEY_GROUP_P.get());
        commonSecretBytes = MpiToBytes(commonSecret);

        return true;
    }

    QByteArray DhIetf1024Sha256Aes128CbcPkcs7::hkdf(const QByteArray& IKM)
    {
        // HKDF-Extract(salt, IKM) -> PRK
        // PRK = HMAC-Hash(salt, IKM)

        // we use NULL salt as per spec
        auto PRK = CryptoHash::hmac(IKM,
                                    QByteArrayLiteral("\0\0\0\0\0\0\0\0"
                                                      "\0\0\0\0\0\0\0\0"
                                                      "\0\0\0\0\0\0\0\0"
                                                      "\0\0\0\0\0\0\0\0"),
                                    CryptoHash::Sha256);

        // HKDF-Expand(PRK, info, L) -> OKM
        // N = ceil(L/HashLen)
        // T = T(1) | T(2) | T(3) | ... | T(N)
        // OKM = first L octets of T
        // where:
        //   T(0) = empty string (zero length)
        //   T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
        //   T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
        //   T(3) = HMAC-Hash(PRK, T(2) | info | 0x03)
        //   ...
        //
        //   (where the constant concatenated to the end of each T(n) is a
        //   single octet.)

        // we use empty info as per spec
        // HashLen = 32 (sha256)
        // L = 16 (16 * 8 = 128 bits)
        // N = ceil(16/32) = 1

        auto T1 = CryptoHash::hmac(QByteArrayLiteral("\x01"), PRK, CryptoHash::Sha256);

        // resulting AES key is first 128 bits
        Q_ASSERT(T1.size() >= AES_KEY_LEN);
        auto OKM = T1.left(AES_KEY_LEN);
        return OKM;
    }

    SecretStruct DhIetf1024Sha256Aes128CbcPkcs7::encrypt(const SecretStruct& input)
    {
        SecretStruct output = input;
        output.value.clear();
        output.parameters.clear();

        SymmetricCipher encrypter(SymmetricCipher::Aes128, SymmetricCipher::Cbc, SymmetricCipher::Encrypt);

        auto IV = randomGen()->randomArray(SymmetricCipher::algorithmIvSize(SymmetricCipher::Aes128));
        if (!encrypter.init(m_aesKey, IV)) {
            qWarning() << "Error encrypt: " << encrypter.errorString();
            return output;
        }

        output.parameters = IV;

        bool ok;
        output.value = input.value;
        output.value = encrypter.process(padPkcs7(output.value, encrypter.blockSize()), &ok);
        if (!ok) {
            qWarning() << "Error encrypt: " << encrypter.errorString();
            return output;
        }

        return output;
    }

    QByteArray& DhIetf1024Sha256Aes128CbcPkcs7::padPkcs7(QByteArray& input, int blockSize)
    {
        // blockSize must be a power of 2.
        Q_ASSERT_X(blockSize > 0 && !(blockSize & (blockSize - 1)), "padPkcs7", "blockSize must be a power of 2");

        int padLen = blockSize - (input.size() & (blockSize - 1));

        input.append(QByteArray(padLen, static_cast<char>(padLen)));
        return input;
    }

    SecretStruct DhIetf1024Sha256Aes128CbcPkcs7::decrypt(const SecretStruct& input)
    {
        auto IV = input.parameters;
        SymmetricCipher decrypter(SymmetricCipher::Aes128, SymmetricCipher::Cbc, SymmetricCipher::Decrypt);
        if (!decrypter.init(m_aesKey, IV)) {
            qWarning() << "Error decoding: " << decrypter.errorString();
            return input;
        }
        bool ok;
        SecretStruct output = input;
        output.parameters.clear();
        output.value = decrypter.process(input.value, &ok);

        if (!ok) {
            qWarning() << "Error decoding: " << decrypter.errorString();
            return input;
        }

        unpadPkcs7(output.value);
        return output;
    }

    QByteArray& DhIetf1024Sha256Aes128CbcPkcs7::unpadPkcs7(QByteArray& input)
    {
        if (input.isEmpty()) {
            return input;
        }

        int padLen = input[input.size() - 1];
        input.chop(padLen);
        return input;
    }

    bool DhIetf1024Sha256Aes128CbcPkcs7::isValid() const
    {
        return m_valid;
    }

    QVariant DhIetf1024Sha256Aes128CbcPkcs7::negotiationOutput() const
    {
        return m_publicKey;
    }

    void DhIetf1024Sha256Aes128CbcPkcs7::fixNextServerKeys(GcryptMPI priv, GcryptMPI pub)
    {
        NextPrivKey = std::move(priv);
        NextPubKey = std::move(pub);
    }

    GcryptMPI DhIetf1024Sha256Aes128CbcPkcs7::NextPrivKey = nullptr;
    GcryptMPI DhIetf1024Sha256Aes128CbcPkcs7::NextPubKey = nullptr;

} // namespace FdoSecrets
