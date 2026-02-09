/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "PasskeyUtils.h"
#include "BrowserMessageBuilder.h"
#include "BrowserPasskeys.h"
#include "core/EntryAttributes.h"
#include "core/Tools.h"
#include "crypto/Random.h"
#include "gui/UrlTools.h"

#include <botan/kdf.h>

#include <QList>
#include <QUrl>

#define CHALLENGE_MIN_LENGTH 16
#define PRF_DERIVED_KEY_LENGTH 32
#define PRF_SECRET_LENGTH 32
#define USER_ID_MIN_LENGTH 1
#define USER_ID_MAX_LENGTH 64

Q_GLOBAL_STATIC(PasskeyUtils, s_passkeyUtils);

PasskeyUtils* PasskeyUtils::instance()
{
    return s_passkeyUtils;
}

int PasskeyUtils::checkLimits(const QJsonObject& pkOptions) const
{
    const auto challenge = pkOptions["challenge"].toString();
    if (challenge.isEmpty() || challenge.length() < CHALLENGE_MIN_LENGTH) {
        return ERROR_PASSKEYS_INVALID_CHALLENGE;
    }

    const auto userIdBase64 = pkOptions["user"]["id"].toString();
    const auto userId = browserMessageBuilder()->getArrayFromBase64(userIdBase64);
    if (userId.isEmpty() || (userId.length() < USER_ID_MIN_LENGTH || userId.length() > USER_ID_MAX_LENGTH)) {
        return ERROR_PASSKEYS_INVALID_USER_ID;
    }

    return PASSKEYS_SUCCESS;
}

// Basic check for the object that it contains necessary variables in a correct form
bool PasskeyUtils::checkCredentialCreationOptions(const QJsonObject& credentialCreationOptions) const
{
    if (!credentialCreationOptions["attestation"].isString()
        || credentialCreationOptions["attestation"].toString().isEmpty()
        || !credentialCreationOptions["clientDataJSON"].isString()
        || credentialCreationOptions["clientDataJSON"].toString().isEmpty()
        || !credentialCreationOptions["rp"].isObject() || credentialCreationOptions["rp"].toObject().isEmpty()
        || !credentialCreationOptions["user"].isObject() || credentialCreationOptions["user"].toObject().isEmpty()
        || !credentialCreationOptions["residentKey"].isBool() || credentialCreationOptions["residentKey"].isUndefined()
        || !credentialCreationOptions["userPresence"].isBool()
        || credentialCreationOptions["userPresence"].isUndefined()
        || !credentialCreationOptions["userVerification"].isBool()
        || credentialCreationOptions["userVerification"].isUndefined()
        || !credentialCreationOptions["credTypesAndPubKeyAlgs"].isArray()
        || credentialCreationOptions["credTypesAndPubKeyAlgs"].toArray().isEmpty()
        || !credentialCreationOptions["excludeCredentials"].isArray()
        || credentialCreationOptions["excludeCredentials"].isUndefined()) {
        return false;
    }

    return true;
}

// Basic check for the object that it contains necessary variables in a correct form
bool PasskeyUtils::checkCredentialAssertionOptions(const QJsonObject& assertionOptions) const
{
    if (!assertionOptions["clientDataJson"].isString() || assertionOptions["clientDataJson"].toString().isEmpty()
        || !assertionOptions["rpId"].isString() || assertionOptions["rpId"].toString().isEmpty()
        || !assertionOptions["userPresence"].isBool() || assertionOptions["userPresence"].isUndefined()
        || !assertionOptions["userVerification"].isBool() || assertionOptions["userVerification"].isUndefined()) {
        return false;
    }

    return true;
}

int PasskeyUtils::checkPrfEvalByCredential(const QJsonObject& assertionOptions,
                                           const QJsonObject& extensionObject) const
{
    const auto prfObject = extensionObject["prf"].toObject();
    if (!prfObject.isEmpty() && prfObject.contains("evalByCredential")) {
        const auto evalByCredential = prfObject["evalByCredential"].toObject();
        const auto allowedCredentials = passkeyUtils()->getAllowedCredentialsFromAssertionOptions(assertionOptions);

        // Cannot be empty if evalByCredential is not empty
        if (allowedCredentials.isEmpty() && !evalByCredential.isEmpty()) {
            return ERROR_PASSKEYS_EVAL_BY_CREDENTIAL_NOT_EMPTY;
        }

        // evalByCredential must include a key that is one of allowedCredentials
        bool credentialFound = false;
        for (const auto& credential : allowedCredentials) {
            if (evalByCredential.contains(credential)) {
                credentialFound = true;
            }
        }

        if (!credentialFound) {
            return ERROR_PASSKEYS_EVAL_BY_CREDENTIAL_NOT_FOUND;
        }
    }

    return PASSKEYS_SUCCESS;
}

int PasskeyUtils::getEffectiveDomain(const QString& origin, QString* result) const
{
    if (!result) {
        return ERROR_PASSKEYS_ORIGIN_NOT_ALLOWED;
    }

    if (origin.isEmpty()) {
        return ERROR_PASSKEYS_ORIGIN_NOT_ALLOWED;
    }

    const auto effectiveDomain = QUrl::fromUserInput(origin).host();
    if (!isDomain(effectiveDomain)) {
        return ERROR_PASSKEYS_DOMAIN_IS_NOT_VALID;
    }

    *result = effectiveDomain;
    return PASSKEYS_SUCCESS;
}

int PasskeyUtils::validateRpId(const QJsonValue& rpIdValue, const QString& effectiveDomain, QString* result) const
{
    if (!result) {
        return ERROR_PASSKEYS_DOMAIN_RPID_MISMATCH;
    }

    if (effectiveDomain.isEmpty()) {
        return ERROR_PASSKEYS_ORIGIN_NOT_ALLOWED;
    }

    //  The RP ID defaults to being the caller's origin's effective domain unless the caller has explicitly set
    //  options.rp.id
    if (rpIdValue.isUndefined() || rpIdValue.isNull()) {
        *result = effectiveDomain;
        return PASSKEYS_SUCCESS;
    }

    const auto rpId = rpIdValue.toString();
    if (!isRegistrableDomainSuffix(rpId, effectiveDomain)) {
        return ERROR_PASSKEYS_DOMAIN_RPID_MISMATCH;
    }

    if (rpId == effectiveDomain) {
        *result = effectiveDomain;
        return PASSKEYS_SUCCESS;
    }

    *result = rpId;
    return PASSKEYS_SUCCESS;
}

// https://www.w3.org/TR/2021/REC-webauthn-2-20210408/#dom-publickeycredentialcreationoptions-attestation
QString PasskeyUtils::parseAttestation(const QString& attestation) const
{
    return attestation == BrowserPasskeys::PASSKEYS_ATTESTATION_DIRECT ? BrowserPasskeys::PASSKEYS_ATTESTATION_DIRECT
                                                                       : BrowserPasskeys::PASSKEYS_ATTESTATION_NONE;
}

QJsonArray PasskeyUtils::parseCredentialTypes(const QJsonArray& credentialTypes) const
{
    QJsonArray credTypesAndPubKeyAlgs;

    if (credentialTypes.isEmpty()) {
        // Set default values
        credTypesAndPubKeyAlgs.push_back(QJsonObject({
            {"type", BrowserPasskeys::PUBLIC_KEY},
            {"alg", WebAuthnAlgorithms::ES256},
        }));
        credTypesAndPubKeyAlgs.push_back(QJsonObject({
            {"type", BrowserPasskeys::PUBLIC_KEY},
            {"alg", WebAuthnAlgorithms::RS256},
        }));
    } else {
        for (const auto current : credentialTypes) {
            const auto currentObject = current.toObject();
            if (currentObject["type"] != BrowserPasskeys::PUBLIC_KEY || currentObject["alg"].isUndefined()) {
                continue;
            }

            const auto currentAlg = currentObject["alg"].toInt();
            if (currentAlg != WebAuthnAlgorithms::ES256 && currentAlg != WebAuthnAlgorithms::RS256
                && currentAlg != WebAuthnAlgorithms::EDDSA) {
                continue;
            }

            credTypesAndPubKeyAlgs.push_back(QJsonObject({
                {"type", currentObject["type"]},
                {"alg", currentAlg},
            }));
        }
    }

    return credTypesAndPubKeyAlgs;
}

bool PasskeyUtils::isAuthenticatorSelectionValid(const QJsonObject& authenticatorSelection) const
{
    const auto authenticatorAttachment = authenticatorSelection["authenticatorAttachment"].toString();
    if (!authenticatorAttachment.isEmpty() && authenticatorAttachment != BrowserPasskeys::ATTACHMENT_PLATFORM
        && authenticatorAttachment != BrowserPasskeys::ATTACHMENT_CROSS_PLATFORM) {
        return false;
    }

    const auto requireResidentKey = authenticatorSelection["requireResidentKey"].toBool();
    if (requireResidentKey && !BrowserPasskeys::SUPPORT_RESIDENT_KEYS) {
        return false;
    }

    const auto residentKey = authenticatorSelection["residentKey"].toString();
    if (residentKey == "required" && !BrowserPasskeys::SUPPORT_RESIDENT_KEYS) {
        return false;
    }

    if (residentKey.isEmpty() && requireResidentKey && !BrowserPasskeys::SUPPORT_RESIDENT_KEYS) {
        return false;
    }

    const auto userVerification = authenticatorSelection["userVerification"].toBool();
    if (userVerification && !BrowserPasskeys::SUPPORT_USER_VERIFICATION) {
        return false;
    }

    return true;
}

bool PasskeyUtils::isRegistrableDomainSuffix(const QString& hostSuffixString, const QString& originalHost) const
{
    if (hostSuffixString.isEmpty()) {
        return false;
    }

    if (!isDomain(originalHost)) {
        return false;
    }

    const auto hostSuffix = QUrl::fromUserInput(hostSuffixString).host();
    if (hostSuffix == originalHost) {
        return true;
    }

    if (!isDomain(hostSuffix)) {
        return false;
    }

    const auto prefixedHostSuffix = QString(".%1").arg(hostSuffix);
    if (!originalHost.endsWith(prefixedHostSuffix)) {
        return false;
    }

    if (hostSuffix == UrlTools::getTopLevelDomainFromUrl(hostSuffix)) {
        return false;
    }

    const auto originalPublicSuffix = UrlTools::getTopLevelDomainFromUrl(originalHost);
    if (originalPublicSuffix.isEmpty()) {
        return false;
    }

    if (originalPublicSuffix.endsWith(prefixedHostSuffix)) {
        return false;
    }

    if (!hostSuffix.endsWith(QString(".%1").arg(originalPublicSuffix))) {
        return false;
    }

    return true;
}

bool PasskeyUtils::isDomain(const QString& hostName) const
{
    const auto domain = QUrl::fromUserInput(hostName).host();
    return !domain.isEmpty() && !domain.endsWith('.') && Tools::isAsciiString(domain)
           && !UrlTools::domainHasIllegalCharacters(domain) && !UrlTools::isIpAddress(hostName);
}

bool PasskeyUtils::isUserVerificationValid(const QString& userVerification) const
{
    return QStringList({BrowserPasskeys::REQUIREMENT_PREFERRED,
                        BrowserPasskeys::REQUIREMENT_REQUIRED,
                        BrowserPasskeys::REQUIREMENT_DISCOURAGED})
        .contains(userVerification);
}

bool PasskeyUtils::isOriginAllowedWithLocalhost(bool allowLocalhostWithPasskeys, const QString& origin) const
{
    if (origin.startsWith("https://") || (allowLocalhostWithPasskeys && origin.startsWith("file://"))) {
        return true;
    }

    if (!allowLocalhostWithPasskeys) {
        return false;
    }

    const auto host = QUrl::fromUserInput(origin).host();
    return host == "localhost" || host == "localhost." || host.endsWith(".localhost") || host.endsWith(".localhost.");
}

bool PasskeyUtils::isResidentKeyRequired(const QJsonObject& authenticatorSelection) const
{
    if (authenticatorSelection.isEmpty()) {
        return false;
    }

    const auto residentKey = authenticatorSelection["residentKey"].toString();
    if (residentKey == BrowserPasskeys::REQUIREMENT_REQUIRED
        || (BrowserPasskeys::SUPPORT_RESIDENT_KEYS && residentKey == BrowserPasskeys::REQUIREMENT_PREFERRED)) {
        return true;
    } else if (residentKey == BrowserPasskeys::REQUIREMENT_DISCOURAGED) {
        return false;
    }

    return authenticatorSelection["requireResidentKey"].toBool();
}

bool PasskeyUtils::isUserVerificationRequired(const QJsonObject& authenticatorSelection) const
{
    const auto userVerification = authenticatorSelection["userVerification"].toString();
    return userVerification == BrowserPasskeys::REQUIREMENT_REQUIRED
           || (userVerification == BrowserPasskeys::REQUIREMENT_PREFERRED
               && BrowserPasskeys::SUPPORT_USER_VERIFICATION);
}

ExtensionResult PasskeyUtils::buildExtensionData(QJsonObject& extensionObject,
                                                 const QString& prfSecret,
                                                 const QStringList& allowCredentials) const
{
    const QStringList allowedKeys = {"credProps", "prf", "uvm"};

    // Remove unsupported keys
    for (const auto& key : extensionObject.keys()) {
        if (!allowedKeys.contains(key)) {
            extensionObject.remove(key);
        }
    }

    // Create response object
    QJsonObject extensionJSON;
    ExtensionResult result;

    // https://w3c.github.io/webauthn/#sctn-authenticator-credential-properties-extension
    if (extensionObject.contains("credProps") && extensionObject["credProps"].toBool()) {
        extensionJSON["credProps"] = QJsonObject({{"rk", true}});
    }

    // https://w3c.github.io/webauthn/#sctn-uvm-extension
    if (extensionObject.contains("uvm") && extensionObject["uvm"].toBool()) {
        QJsonArray uvmResponse;
        QJsonArray uvmArray = {
            1, // userVerificationMethod (USER_VERIFY_PRESENCE_INTERNAL "presence_internal", 0x00000001)
            1, // keyProtectionType (KEY_PROTECTION_SOFTWARE "software", 0x0001)
            1, // matcherProtectionType (MATCHER_PROTECTION_SOFTWARE "software", 0x0001)
        };
        uvmResponse.append(uvmArray);
        extensionJSON["uvm"] = uvmResponse;
    }

    // https://www.w3.org/TR/webauthn-3/#prf-extension
    if (extensionObject.contains("prf")) {
        const auto prfResponse = getPrfResponse(extensionObject, prfSecret, {}, allowCredentials);
        extensionJSON["prf"] = prfResponse.response;
        if (prfSecret.isEmpty()) {
            result.prfSecret = prfResponse.secret;
        }
    }

    if (extensionJSON.isEmpty()) {
        return {};
    }

    auto extensionData = m_browserCbor.cborEncodeExtensionData(extensionObject);
    if (!extensionData.isEmpty()) {
        result.extensionData = extensionData;
        result.extensionObject = extensionJSON;
        return result;
    }

    return {};
}

// Serialization order: https://w3c.github.io/webauthn/#clientdatajson-serialization
QString PasskeyUtils::buildClientDataJson(const QJsonObject& publicKey, const QString& origin, bool get) const
{
    return QString("{\"type\":\"%1\",\"challenge\":\"%2\",\"origin\":\"%3\",\"crossOrigin\":false}")
        .arg((get ? QString("webauthn.get") : QString("webauthn.create")), publicKey["challenge"].toString(), origin);
}

QStringList PasskeyUtils::getAllowedCredentialsFromAssertionOptions(const QJsonObject& assertionOptions) const
{
    QStringList allowedCredentials;
    for (const auto& credential : assertionOptions["allowCredentials"].toArray()) {
        const auto cred = credential.toObject();
        const auto id = cred["id"].toString();
        const auto transports = cred["transports"].toArray();
        const auto hasSupportedTransport = transports.isEmpty()
                                           || (transports.contains(BrowserPasskeys::AUTHENTICATOR_TRANSPORT_INTERNAL)
                                               || transports.contains(BrowserPasskeys::AUTHENTICATOR_TRANSPORT_NFC)
                                               || transports.contains(BrowserPasskeys::AUTHENTICATOR_TRANSPORT_USB));

        if (cred["type"].toString() == BrowserPasskeys::PUBLIC_KEY && hasSupportedTransport && !id.isEmpty()) {
            allowedCredentials << id;
        }
    }

    return allowedCredentials;
}

// For compatibility with StrongBox (and other possible clients in the future)
QString PasskeyUtils::getCredentialIdFromEntry(const Entry* entry) const
{
    if (!entry) {
        return {};
    }

    return entry->attributes()->hasKey(EntryAttributes::KPEX_PASSKEY_GENERATED_USER_ID)
               ? entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_GENERATED_USER_ID)
               : entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_CREDENTIAL_ID);
}

// For compatibility with StrongBox (and other possible clients in the future)
QString PasskeyUtils::getUsernameFromEntry(const Entry* entry) const
{
    if (!entry) {
        return {};
    }

    return entry->attributes()->hasKey(EntryAttributes::KPXC_PASSKEY_USERNAME)
               ? entry->attributes()->value(EntryAttributes::KPXC_PASSKEY_USERNAME)
               : entry->attributes()->value(EntryAttributes::KPEX_PASSKEY_USERNAME);
}

// Gets PRF salt directly from the response, or from first matching credential ID in allowCredentials
// https://w3c.github.io/webauthn/#dom-authenticationextensionsprfinputs-eval
QString PasskeyUtils::getPrfSalt(const QJsonObject& prfObject, const QStringList& allowCredentials) const
{
    if (prfObject.contains("eval")) {
        return prfObject["eval"]["first"].toString();
    }

    // Look for matching credential ID
    if (prfObject.contains("evalByCredential") && !allowCredentials.isEmpty()) {
        const auto evalObject = prfObject["evalByCredential"].toObject();
        for (const auto& credential : allowCredentials) {
            const auto salt = evalObject.value(credential).toObject()["first"].toString();
            if (!salt.isEmpty()) {
                return salt;
            }
        }
    }

    return {};
}

PrfResult PasskeyUtils::getPrfResponse(const QJsonObject& extensionObject,
                                       const QString& prfSecret,
                                       const QString& label,
                                       const QStringList& allowCredentials) const
{
    PrfResult result;

    const auto prfObject = extensionObject["prf"].toObject();
    const auto salt = getPrfSalt(prfObject, allowCredentials);

    if (salt.isEmpty()) {
        // Only the feature capability was requested
        QJsonObject prfResponse{
            {"enabled", true},
        };
        result.response = prfResponse;
        return result;
    }

    // Salt was provided (must be: UTF8Encode("WebAuthn PRF") || 0x00 || eval.first)
    auto saltBytes =
        QByteArray("WebAuthn PRF") + QByteArray::fromHex("00") + browserMessageBuilder()->getArrayFromBase64(salt);
    auto secret = prfSecret.isEmpty() ? randomGen()->randomArray(PRF_SECRET_LENGTH)
                                      : browserMessageBuilder()->getArrayFromBase64(prfSecret);

    auto kdf = Botan::KDF::create_or_throw("HKDF(SHA-256)");
    const Botan::secure_vector<uint8_t> vecSecret(secret.begin(), secret.end());
    const std::vector<uint8_t> vecSalt(saltBytes.begin(), saltBytes.end());

    std::vector<uint8_t> vecLabel = {};
    if (!label.isEmpty()) {
        const auto labelBytes = browserMessageBuilder()->getArrayFromBase64(label);
        vecLabel = std::vector<uint8_t>(labelBytes.begin(), labelBytes.end());
    }

    const size_t derived_key_len = PRF_DERIVED_KEY_LENGTH;
    const auto derivedKey = kdf->derive_key(derived_key_len, vecSecret, vecSalt, vecLabel);
    const auto keyData = browserMessageBuilder()->getBase64FromKey(derivedKey.data(), derived_key_len);

    QJsonObject prfResponse{
        {"results", QJsonObject{{"first", keyData}}},
    };

    result.response = prfResponse;
    if (prfSecret.isEmpty()) {
        // New secret was generated
        result.secret = browserMessageBuilder()->getBase64FromArray(secret);
    }

    return result;
}
