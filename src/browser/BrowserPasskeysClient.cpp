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

#include "BrowserPasskeysClient.h"
#include "BrowserMessageBuilder.h"
#include "BrowserPasskeys.h"
#include "PasskeyUtils.h"

#include <QJsonDocument>

Q_GLOBAL_STATIC(BrowserPasskeysClient, s_browserPasskeysClient);

BrowserPasskeysClient* BrowserPasskeysClient::instance()
{
    return s_browserPasskeysClient;
}

// Constructs CredentialCreationOptions from the original PublicKeyCredential
// https://www.w3.org/TR/2019/REC-webauthn-1-20190304/#createCredential
int BrowserPasskeysClient::getCredentialCreationOptions(const QJsonObject& publicKeyOptions,
                                                        const QString& origin,
                                                        QJsonObject* result) const
{
    if (!result || publicKeyOptions.isEmpty()) {
        return ERROR_PASSKEYS_EMPTY_PUBLIC_KEY;
    }

    // Check validity of some basic values
    const auto checkResultError = passkeyUtils()->checkLimits(publicKeyOptions);
    if (checkResultError > 0) {
        return checkResultError;
    }

    // Get effective domain
    QString effectiveDomain;
    const auto effectiveDomainResponse = passkeyUtils()->getEffectiveDomain(origin, &effectiveDomain);
    if (effectiveDomainResponse > 0) {
        return effectiveDomainResponse;
    }

    // Validate RP ID
    QString rpId;
    const auto rpName = publicKeyOptions["rp"]["name"].toString();
    const auto rpIdResponse = passkeyUtils()->validateRpId(publicKeyOptions["rp"]["id"], effectiveDomain, &rpId);
    if (rpIdResponse > 0) {
        return rpIdResponse;
    }

    // Check PublicKeyCredentialTypes
    const auto pubKeyCredParams = passkeyUtils()->parseCredentialTypes(publicKeyOptions["pubKeyCredParams"].toArray());
    if (pubKeyCredParams.isEmpty() && !publicKeyOptions["pubKeyCredParams"].toArray().isEmpty()) {
        return ERROR_PASSKEYS_NO_SUPPORTED_ALGORITHMS;
    }

    // Check Attestation
    const auto attestation = passkeyUtils()->parseAttestation(publicKeyOptions["attestation"].toString());

    // Check validity of AuthenticatorSelection
    auto authenticatorSelection = publicKeyOptions["authenticatorSelection"].toObject();
    const bool isAuthenticatorSelectionValid = passkeyUtils()->isAuthenticatorSelectionValid(authenticatorSelection);
    if (!isAuthenticatorSelectionValid) {
        return ERROR_PASSKEYS_WAIT_FOR_LIFETIMER;
    }

    // Add default values for compatibility
    if (authenticatorSelection.isEmpty()) {
        authenticatorSelection = QJsonObject({{"userVerification", BrowserPasskeys::REQUIREMENT_PREFERRED}});
    } else if (authenticatorSelection["userVerification"].toString().isEmpty()) {
        authenticatorSelection["userVerification"] = BrowserPasskeys::REQUIREMENT_PREFERRED;
    }

    auto authenticatorAttachment = authenticatorSelection["authenticatorAttachment"].toString();
    if (authenticatorAttachment.isEmpty()) {
        authenticatorAttachment = BrowserPasskeys::ATTACHMENT_PLATFORM;
    }

    // Unknown values are ignored, but a warning will be still shown just in case
    const auto userVerification = authenticatorSelection["userVerification"].toString();
    if (!passkeyUtils()->isUserVerificationValid(userVerification)) {
        qWarning() << browserMessageBuilder()->getErrorMessage(ERROR_PASSKEYS_INVALID_USER_VERIFICATION);
    }

    // Parse requireResidentKey and userVerification
    const auto isResidentKeyRequired = passkeyUtils()->isResidentKeyRequired(authenticatorSelection);
    const auto isUserVerificationRequired = passkeyUtils()->isUserVerificationRequired(authenticatorSelection);

    // Extensions
    auto extensionObject = publicKeyOptions["extensions"].toObject();
    const auto extensionData = passkeyUtils()->buildExtensionData(extensionObject);
    const auto extensions = browserMessageBuilder()->getBase64FromArray(extensionData);

    // Construct the final object
    QJsonObject credentialCreationOptions;
    credentialCreationOptions["attestation"] = attestation; // Set this, even if only "none" is supported
    credentialCreationOptions["authenticatorAttachment"] = authenticatorAttachment;
    credentialCreationOptions["clientDataJSON"] = passkeyUtils()->buildClientDataJson(publicKeyOptions, origin, false);
    credentialCreationOptions["credTypesAndPubKeyAlgs"] = pubKeyCredParams;
    credentialCreationOptions["excludeCredentials"] = publicKeyOptions["excludeCredentials"];
    credentialCreationOptions["extensions"] = extensions;
    credentialCreationOptions["residentKey"] = isResidentKeyRequired;
    credentialCreationOptions["rp"] = QJsonObject({{"id", rpId}, {"name", rpName}});
    credentialCreationOptions["user"] = publicKeyOptions["user"];
    credentialCreationOptions["userPresence"] = !isUserVerificationRequired;
    credentialCreationOptions["userVerification"] = isUserVerificationRequired;

    *result = credentialCreationOptions;
    return 0;
}

// Use an existing credential
// https://www.w3.org/TR/2019/REC-webauthn-1-20190304/#getAssertion
int BrowserPasskeysClient::getAssertionOptions(const QJsonObject& publicKeyOptions,
                                               const QString& origin,
                                               QJsonObject* result) const
{
    if (!result || publicKeyOptions.isEmpty()) {
        return ERROR_PASSKEYS_EMPTY_PUBLIC_KEY;
    }

    // Get effective domain
    QString effectiveDomain;
    const auto effectiveDomainResponse = passkeyUtils()->getEffectiveDomain(origin, &effectiveDomain);
    if (effectiveDomainResponse > 0) {
        return effectiveDomainResponse;
    }

    // Validate RP ID
    QString rpId;
    const auto rpIdResponse = passkeyUtils()->validateRpId(publicKeyOptions["rpId"], effectiveDomain, &rpId);
    if (rpIdResponse > 0) {
        return rpIdResponse;
    }

    // Extensions
    auto extensionObject = publicKeyOptions["extensions"].toObject();
    const auto extensionData = passkeyUtils()->buildExtensionData(extensionObject);
    const auto extensions = browserMessageBuilder()->getBase64FromArray(extensionData);

    // clientDataJson
    const auto clientDataJson = passkeyUtils()->buildClientDataJson(publicKeyOptions, origin, true);

    // Unknown values are ignored, but a warning will be still shown just in case
    const auto userVerification = publicKeyOptions["userVerification"].toString();
    if (!passkeyUtils()->isUserVerificationValid(userVerification)) {
        qWarning() << browserMessageBuilder()->getErrorMessage(ERROR_PASSKEYS_INVALID_USER_VERIFICATION);
    }
    const auto isUserVerificationRequired = passkeyUtils()->isUserVerificationRequired(publicKeyOptions);

    QJsonObject assertionOptions;
    assertionOptions["allowCredentials"] = publicKeyOptions["allowCredentials"];
    assertionOptions["clientDataJson"] = clientDataJson;
    assertionOptions["extensions"] = extensions;
    assertionOptions["rpId"] = rpId;
    assertionOptions["userPresence"] = true;
    assertionOptions["userVerification"] = isUserVerificationRequired;

    *result = assertionOptions;
    return 0;
}
