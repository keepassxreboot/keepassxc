/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "Signature.h"

#include "crypto/Random.h"

#include <botan/pubkey.h>

bool Signature::create(const QByteArray& data, QSharedPointer<Botan::Private_Key> key, QString& signature)
{
    // TODO HNH: currently we publish the signature in our own non-standard format - it would
    //           be better to use a standard format (like ASN1 - but this would be more easy
    //           when we integrate a proper library)
    //           Even more, we could publish standard self signed certificates with the container
    //           instead of the custom certificates
    if (key->algo_name() == "RSA") {
        try {
            Botan::PK_Signer signer(*key, "EMSA3(SHA-256)");
            signer.update(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
            auto s = signer.signature(*randomGen()->getRng());

            auto hex = QByteArray(reinterpret_cast<char*>(s.data()), s.size()).toHex();
            signature = QString("rsa|%1").arg(QString::fromLatin1(hex));
            return true;
        } catch (std::exception& e) {
            qWarning("KeeShare: Failed to sign data: %s", e.what());
            return false;
        }
    }
    qWarning("Unsupported Public/Private key format");
    return false;
}

bool Signature::verify(const QByteArray& data, QSharedPointer<Botan::Public_Key> key, const QString& signature)
{
    if (key && key->algo_name() == "RSA") {
        QRegExp extractor("rsa\\|([a-f0-9]+)", Qt::CaseInsensitive);
        if (!extractor.exactMatch(signature) || extractor.captureCount() != 1) {
            qWarning("Could not unpack signature parts");
            return false;
        }
        const QByteArray sig_s = QByteArray::fromHex(extractor.cap(1).toLatin1());

        try {
            Botan::PK_Verifier verifier(*key, "EMSA3(SHA-256)");
            verifier.update(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
            return verifier.check_signature(reinterpret_cast<const uint8_t*>(sig_s.constData()), sig_s.size());
        } catch (std::exception& e) {
            qWarning("KeeShare: Failed to verify signature: %s", e.what());
            return false;
        }
    }
    qWarning("Unsupported Public/Private key format");
    return false;
}
