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

#ifndef KEEPASSXC_BROWSERCBOR_H
#define KEEPASSXC_BROWSERCBOR_H

#include <QByteArray>
#include <QCborArray>
#include <QCborMap>
#include <QJsonObject>

enum WebAuthnAlgorithms : int
{
    ES256 = -7,
    EDDSA = -8,
    ES384 = -35,
    ES512 = -36,
    RS256 = -257
};

// https://www.rfc-editor.org/rfc/rfc9053#section-7.1
enum WebAuthnCurveKey : int
{
    INVALID_CURVE_KEY = 0,
    P256 = 1, // EC2, NIST P-256, also known as secp256r1
    P384 = 2, // EC2, NIST P-384, also known as secp384r1
    P521 = 3, // EC2, NIST P-521, also known as secp521r1
    X25519 = 4, // OKP, X25519 for use w/ ECDH only
    X448 = 5, // OKP, X448 for use w/ ECDH only
    ED25519 = 6, // OKP, Ed25519 for use w/ EdDSA only
    ED448 = 7 // OKP, Ed448 for use w/ EdDSA only
};

// https://www.rfc-editor.org/rfc/rfc8152
// For RSA: https://www.rfc-editor.org/rfc/rfc8230#section-4
enum WebAuthnCoseKeyType : int
{
    INVALID_COSE_KEY_TYPE = 0,
    OKP = 1, // Octet Keypair
    EC2 = 2, // Elliptic Curve
    RSA = 3 // RSA
};

class BrowserCbor
{
public:
    QByteArray cborEncodeAttestation(const QByteArray& authData) const;
    QByteArray cborEncodePublicKey(int alg, const QByteArray& first, const QByteArray& second) const;
    QByteArray cborEncodeExtensionData(const QJsonObject& extensions) const;
    QJsonObject getJsonFromCborData(const QByteArray& byteArray) const;
    QVariant handleCborArray(const QCborArray& array) const;
    QVariant handleCborMap(const QCborMap& map) const;
    QVariant handleCborValue(const QCborValue& value) const;
    unsigned int getCoseKeyType(int alg) const;
    unsigned int getCurveParameter(int alg) const;
};

#endif // KEEPASSXC_BROWSERCBOR_H
