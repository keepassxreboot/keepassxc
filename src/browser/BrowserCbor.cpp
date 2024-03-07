/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BrowserCbor.h"
#include "BrowserMessageBuilder.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QJsonDocument>

// https://w3c.github.io/webauthn/#sctn-none-attestation
// https://w3c.github.io/webauthn/#sctn-generating-an-attestation-object
QByteArray BrowserCbor::cborEncodeAttestation(const QByteArray& authData) const
{
    QByteArray result;
    QCborStreamWriter writer(&result);

    writer.startMap(3);

    writer.append("fmt");
    writer.append("none");

    writer.append("attStmt");
    writer.startMap(0);
    writer.endMap();

    writer.append("authData");
    writer.appendByteString(authData.constData(), authData.size());

    writer.endMap();

    return result;
}

// https://w3c.github.io/webauthn/#authdata-attestedcredentialdata-credentialpublickey
QByteArray BrowserCbor::cborEncodePublicKey(int alg, const QByteArray& first, const QByteArray& second) const
{
    const auto keyType = getCoseKeyType(alg);
    if (keyType == 0) {
        return {};
    }

    const auto curveParameter = getCurveParameter(alg);
    if ((alg == WebAuthnAlgorithms::ES256 || alg == WebAuthnAlgorithms::EDDSA) && curveParameter == 0) {
        return {};
    }

    QByteArray result;
    QCborStreamWriter writer(&result);

    if (alg == WebAuthnAlgorithms::ES256) {
        writer.startMap(5);

        // Key type
        writer.append(1);
        writer.append(keyType);

        // Signature algorithm
        writer.append(3);
        writer.append(alg);

        // Curve parameter
        writer.append(-1);
        writer.append(curveParameter);

        // Key x-coordinate
        writer.append(-2);
        writer.append(first);

        // Key y-coordinate
        writer.append(-3);
        writer.append(second);

        writer.endMap();
    } else if (alg == WebAuthnAlgorithms::RS256) {
        writer.startMap(4);

        // Key type
        writer.append(1);
        writer.append(keyType);

        // Signature algorithm
        writer.append(3);
        writer.append(alg);

        // Key modulus
        writer.append(-1);
        writer.append(first);

        // Key exponent
        writer.append(-2);
        writer.append(second);

        writer.endMap();
    } else if (alg == WebAuthnAlgorithms::EDDSA) {
        writer.startMap(4);

        // Key type
        writer.append(1);
        writer.append(keyType);

        // Algorithm
        writer.append(3);
        writer.append(alg);

        // Curve parameter
        writer.append(-1);
        writer.append(curveParameter);

        // Public key
        writer.append(-2);
        writer.append(first);

        writer.endMap();
    }

    return result;
}

// See: https://fidoalliance.org/specs/common-specs/fido-registry-v2.1-ps-20191217.html#user-verification-methods
QByteArray BrowserCbor::cborEncodeExtensionData(const QJsonObject& extensions) const
{
    if (extensions.empty()) {
        return {};
    }

    QByteArray result;
    QCborStreamWriter writer(&result);

    writer.startMap(extensions.keys().count());
    if (extensions["credProps"].toBool()) {
        writer.append("credProps");
        writer.startMap(1);
        writer.append("rk");
        writer.append(true);
        writer.endMap();
    }

    if (extensions["uvm"].toBool()) {
        writer.append("uvm");

        writer.startArray(1);
        writer.startArray(3);

        // userVerificationMethod (USER_VERIFY_PRESENCE_INTERNAL "presence_internal", 0x00000001)
        writer.append(quint32(1));

        // keyProtectionType (KEY_PROTECTION_SOFTWARE "software", 0x0001)
        writer.append(quint16(1));

        // matcherProtectionType (MATCHER_PROTECTION_SOFTWARE "software", 0x0001)
        writer.append(quint16(1));

        writer.endArray();
        writer.endArray();
    }
    writer.endMap();

    return result;
}

QJsonObject BrowserCbor::getJsonFromCborData(const QByteArray& byteArray) const
{
    auto reader = QCborStreamReader(byteArray);
    auto contents = QCborValue::fromCbor(reader);
    if (reader.lastError()) {
        return {};
    }

    const auto ret = handleCborValue(contents);

    // Parse variant result to QJsonDocument
    const auto jsonDocument = QJsonDocument::fromVariant(ret);
    if (jsonDocument.isNull() || jsonDocument.isEmpty()) {
        return {};
    }

    return jsonDocument.object();
}

QVariant BrowserCbor::handleCborArray(const QCborArray& array) const
{
    QVariantList result;
    result.reserve(array.size());

    for (auto a : array) {
        result.append(handleCborValue(a));
    }

    return result;
}

QVariant BrowserCbor::handleCborMap(const QCborMap& map) const
{
    QVariantMap result;
    for (auto pair : map) {
        result.insert(handleCborValue(pair.first).toString(), handleCborValue(pair.second));
    }

    return QVariant::fromValue(result);
}

QVariant BrowserCbor::handleCborValue(const QCborValue& value) const
{
    if (value.isArray()) {
        return handleCborArray(value.toArray());
    } else if (value.isMap()) {
        return handleCborMap(value.toMap());
    } else if (value.isByteArray()) {
        auto ba = value.toByteArray();

        // Return base64 instead of raw byte array
        auto base64Str = browserMessageBuilder()->getBase64FromArray(ba);
        return QVariant::fromValue(base64Str);
    }

    return value.toVariant();
}

// https://www.rfc-editor.org/rfc/rfc8152#section-13.1
unsigned int BrowserCbor::getCurveParameter(int alg) const
{
    switch (alg) {
    case WebAuthnAlgorithms::ES256:
        return WebAuthnCurveKey::P256;
    case WebAuthnAlgorithms::ES384:
        return WebAuthnCurveKey::P384;
    case WebAuthnAlgorithms::ES512:
        return WebAuthnCurveKey::P521;
    case WebAuthnAlgorithms::EDDSA:
        return WebAuthnCurveKey::ED25519;
    default:
        return WebAuthnCurveKey::INVALID_CURVE_KEY;
    }
}

// See: https://www.rfc-editor.org/rfc/rfc8152
// AES/HMAC/ChaCha20 etc. carries symmetric keys (4) and OKP not supported currently.
unsigned int BrowserCbor::getCoseKeyType(int alg) const
{
    switch (alg) {
    case WebAuthnAlgorithms::ES256:
        return WebAuthnCoseKeyType::EC2;
    case WebAuthnAlgorithms::ES384:
    case WebAuthnAlgorithms::ES512:
        return WebAuthnCoseKeyType::INVALID_COSE_KEY_TYPE;
    case WebAuthnAlgorithms::EDDSA:
        return WebAuthnCoseKeyType::OKP;
    case WebAuthnAlgorithms::RS256:
        return WebAuthnCoseKeyType::RSA;
    default:
        return WebAuthnCoseKeyType::INVALID_COSE_KEY_TYPE;
    }
}
