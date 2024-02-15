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

#include "AuthenticationFactorGroup.h"

void AuthenticationFactorGroup::setValidationIn(const QByteArray& validationIn)
{
    m_validationIn = validationIn;
}

void AuthenticationFactorGroup::setValidationOut(const QByteArray& validationOut)
{
    m_validationOut = validationOut;
}

void AuthenticationFactorGroup::setChallenge(const QByteArray& challenge)
{
    m_challenge = challenge;
}

void AuthenticationFactorGroup::setValidationType(const AuthenticationFactorGroupValidationType validationType)
{
    m_validationType = validationType;
}

void AuthenticationFactorGroup::addFactor(const QSharedPointer<AuthenticationFactor>& factor)
{
    m_factors.append(factor);
    factor->setParent(this);
}

const QList<QSharedPointer<AuthenticationFactor>>& AuthenticationFactorGroup::getFactors() const
{
    return m_factors;
}

const QByteArray& AuthenticationFactorGroup::getValidationIn() const
{
    return m_validationIn;
}

const QByteArray& AuthenticationFactorGroup::getValidationOut() const
{
    return m_validationOut;
}

const QByteArray& AuthenticationFactorGroup::getChallenge() const
{
    return m_challenge;
}

AuthenticationFactorGroupValidationType AuthenticationFactorGroup::getValidationType() const
{
    return m_validationType;
}
