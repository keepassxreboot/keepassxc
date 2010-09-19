/*
*  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "PasswordKey.h"

#include "crypto/CryptoHash.h"

PasswordKey::PasswordKey()
{
}

PasswordKey::PasswordKey(const QString& password)
{
    setPassword(password);
}

QByteArray PasswordKey::rawKey() const
{
    return m_key;
}

void PasswordKey::setPassword(const QString& password)
{
    m_key = CryptoHash::hash(password.toUtf8(), CryptoHash::Sha256);
}

PasswordKey* PasswordKey::clone() const
{
    return new PasswordKey(*this);
}
