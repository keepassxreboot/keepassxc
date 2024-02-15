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

#include "AuthenticationFactor.h"

#include <QSharedPointer>
#include <QDebug>

void AuthenticationFactor::setName(const QString& name)
{
    m_name = name;
}

const QString& AuthenticationFactor::getName() const
{
    return m_name;
}

void AuthenticationFactor::setKeyType(AuthenticationFactorKeyType type)
{
    m_keyType = type;
}

void AuthenticationFactor::setKeySalt(const QByteArray& salt)
{
    m_keySalt = salt;
}

void AuthenticationFactor::setWrappedKey(const QByteArray& key)
{
    m_wrappedKey = key;
}

const QByteArray& AuthenticationFactor::getWrappedKey() const
{
    return m_wrappedKey;
}

const QByteArray& AuthenticationFactor::getKeySalt() const
{
    return m_keySalt;
}

AuthenticationFactorKeyType AuthenticationFactor::getKeyType() const
{
    return m_keyType;
}

const QString& AuthenticationFactor::getFactorType() const
{
    return m_factorType;
}
