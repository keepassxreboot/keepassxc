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

#ifndef PASSKEYUTILS_H
#define PASSKEYUTILS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>

#include "BrowserCbor.h"
#include "core/Entry.h"

#define DEFAULT_TIMEOUT 300000
#define DEFAULT_DISCOURAGED_TIMEOUT 120000
#define PASSKEYS_SUCCESS 0

class PasskeyUtils : public QObject
{
    Q_OBJECT

public:
    explicit PasskeyUtils() = default;
    ~PasskeyUtils() = default;
    static PasskeyUtils* instance();

    int checkLimits(const QJsonObject& pkOptions) const;
    bool checkCredentialCreationOptions(const QJsonObject& credentialCreationOptions) const;
    bool checkCredentialAssertionOptions(const QJsonObject& assertionOptions) const;
    int getEffectiveDomain(const QString& origin, QString* result) const;
    int validateRpId(const QJsonValue& rpIdValue, const QString& effectiveDomain, QString* result) const;
    QString parseAttestation(const QString& attestation) const;
    QJsonArray parseCredentialTypes(const QJsonArray& credentialTypes) const;
    bool isAuthenticatorSelectionValid(const QJsonObject& authenticatorSelection) const;
    bool isUserVerificationValid(const QString& userVerification) const;
    bool isResidentKeyRequired(const QJsonObject& authenticatorSelection) const;
    bool isUserVerificationRequired(const QJsonObject& authenticatorSelection) const;
    bool isOriginAllowedWithLocalhost(bool allowLocalhostWithPasskeys, const QString& origin) const;
    QByteArray buildExtensionData(QJsonObject& extensionObject) const;
    QJsonObject buildClientDataJson(const QJsonObject& publicKey, const QString& origin, bool get) const;
    QStringList getAllowedCredentialsFromAssertionOptions(const QJsonObject& assertionOptions) const;
    QString getCredentialIdFromEntry(const Entry* entry) const;
    QString getUsernameFromEntry(const Entry* entry) const;

private:
    Q_DISABLE_COPY(PasskeyUtils);

    bool isRegistrableDomainSuffix(const QString& hostSuffixString, const QString& originalHost) const;
    bool isDomain(const QString& hostName) const;

    friend class TestPasskeys;

private:
    BrowserCbor m_browserCbor;
};

static inline PasskeyUtils* passkeyUtils()
{
    return PasskeyUtils::instance();
}

#endif // PASSKEYUTILS_H
