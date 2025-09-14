/*
 * Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <botan/secmem.h>

#include "core/AuthenticationFactorUserData.h"
#include "format/multifactor/AuthenticationFactor.h"

class AuthenticationFactor;
class AuthenticationFactorInfo;

class AuthenticationFactorGroup
{
public:
    AuthenticationFactorGroup() = default;
    virtual ~AuthenticationFactorGroup() = default;

    enum class ValidationType
    {
        NONE,
        HMAC_SHA512,
    };

    QSharedPointer<QByteArray> getRawKey(QSharedPointer<AuthenticationFactorUserData> userData);

    void setValidationIn(const QByteArray& validationIn);
    QByteArray getValidationIn() const;
    void setValidationOut(const QByteArray& validationOut);
    QByteArray getValidationOut() const;
    void setChallenge(const QByteArray& challenge);
    QByteArray getChallenge() const;
    void setValidationType(ValidationType validationType);
    ValidationType getValidationType() const;
    void addFactor(QSharedPointer<AuthenticationFactor> factor);
    const QList<QSharedPointer<AuthenticationFactor>>& getFactors() const;

protected:
    QByteArray m_validationIn;
    QByteArray m_validationOut;
    QByteArray m_challenge;
    ValidationType m_validationType = ValidationType::NONE;

    QList<QSharedPointer<AuthenticationFactor>> m_factors;

    Botan::secure_vector<char> m_key;
};
