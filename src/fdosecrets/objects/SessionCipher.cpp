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

#include "config-keepassx.h"

#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"

#include <QDebug>
#include <botan/dh.h>

#ifdef WITH_XC_BOTAN3
#include <botan/dl_group.h>
#include <botan/pubkey.h>
#else
#include <botan/pk_ops.h>
#endif

namespace FdoSecrets
{
    // XXX: remove the redundant definitions once we are at C++17
    constexpr char PlainCipher::Algorithm[];
    constexpr char DhIetf1024Sha256Aes128CbcPkcs7::Algorithm[];

    DhIetf1024Sha256Aes128CbcPkcs7::DhIetf1024Sha256Aes128CbcPkcs7(const QByteArray& clientPublicKey)
    {
        try {
            m_privateKey.reset(new Botan::DH_PrivateKey(*randomGen()->getRng(), Botan::DL_Group("modp/ietf/1024")));
            m_valid = updateClientPublicKey(clientPublicKey);
        } catch (std::exception& e) {
            qCritical("Failed to derive DH key: %s", e.what());
            m_privateKey.reset();
            m_valid = false;
        }
    }

    bool DhIetf1024Sha256Aes128CbcPkcs7::updateClientPublicKey(const QByteArray& clientPublicKey)
    {
        if (!m_privateKey) {
            return false;
        }

        try {
            Botan::secure_vector<uint8_t> salt(32, '\0');
#ifdef WITH_XC_BOTAN3
            Botan::PK_Key_Agreement dhka(*m_privateKey, *randomGen()->getRng(), "HKDF(SHA-256)", "");
            auto aesKey = dhka.derive_key(16,
                                          reinterpret_cast<const uint8_t*>(clientPublicKey.constData()),
                                          clientPublicKey.size(),
                                          salt.data(),
                                          salt.size());
            m_aesKey = QByteArray(reinterpret_cast<const char*>(aesKey.begin()), aesKey.size());
#else
            auto dhka = m_privateKey->create_key_agreement_op(*randomGen()->getRng(), "HKDF(SHA-256)", "");
            auto aesKey = dhka->agree(16,
                                      reinterpret_cast<const uint8_t*>(clientPublicKey.constData()),
                                      clientPublicKey.size(),
                                      salt.data(),
                                      salt.size());
            m_aesKey = QByteArray(reinterpret_cast<char*>(aesKey.data()), aesKey.size());
#endif
            return true;
        } catch (std::exception& e) {
            qCritical("Failed to update client public key: %s", e.what());
            return false;
        }
    }

    Secret DhIetf1024Sha256Aes128CbcPkcs7::encrypt(const Secret& input)
    {
        Secret output = input;
        output.parameters.clear();
        output.value.clear();

        SymmetricCipher encrypter;
        auto IV = randomGen()->randomArray(SymmetricCipher::defaultIvSize(SymmetricCipher::Aes128_CBC));
        if (!encrypter.init(SymmetricCipher::Aes128_CBC, SymmetricCipher::Encrypt, m_aesKey, IV)) {
            qWarning() << "Error encrypt: " << encrypter.errorString();
            return output;
        }

        output.parameters = IV;
        output.value = input.value;
        if (!encrypter.finish(output.value)) {
            qWarning() << "Error encrypt: " << encrypter.errorString();
            return output;
        }

        return output;
    }

    Secret DhIetf1024Sha256Aes128CbcPkcs7::decrypt(const Secret& input)
    {
        SymmetricCipher decrypter;
        if (!decrypter.init(SymmetricCipher::Aes128_CBC, SymmetricCipher::Decrypt, m_aesKey, input.parameters)) {
            qWarning() << "Error decoding: " << decrypter.errorString();
            return input;
        }

        Secret output = input;
        output.parameters.clear();
        if (!decrypter.finish(output.value)) {
            qWarning() << "Error decoding: " << decrypter.errorString();
            return input;
        }
        return output;
    }

    bool DhIetf1024Sha256Aes128CbcPkcs7::isValid() const
    {
        return m_valid;
    }

    QVariant DhIetf1024Sha256Aes128CbcPkcs7::negotiationOutput() const
    {
        if (m_valid) {
            auto pubkey = m_privateKey->public_value();
            return QByteArray(reinterpret_cast<char*>(pubkey.data()), pubkey.size());
        }
        return {};
    }
} // namespace FdoSecrets
