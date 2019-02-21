/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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
#include "core/Tools.h"

#include "crypto/CryptoHash.h"
#include <gcrypt.h>
#include <algorithm>
#include <cstring>

QUuid PasswordKey::UUID("77e90411-303a-43f2-b773-853b05635ead");

constexpr int PasswordKey::SHA256_SIZE;

PasswordKey::PasswordKey()
    : Key(UUID)
    , m_key(static_cast<char*>(gcry_malloc_secure(SHA256_SIZE)))
{
}

PasswordKey::PasswordKey(const QString& password)
    : Key(UUID)
    , m_key(static_cast<char*>(gcry_malloc_secure(SHA256_SIZE)))
{
    setPassword(password);
}

PasswordKey::~PasswordKey()
{
    if (m_key) {
        gcry_free(m_key);
        m_key = nullptr;
    }
}

QSharedPointer<PasswordKey> PasswordKey::fromRawKey(const QByteArray& rawKey)
{
    auto result = QSharedPointer<PasswordKey>::create();
    std::memcpy(result->m_key, rawKey.data(), std::min(SHA256_SIZE, rawKey.size()));
    return result;
}

QByteArray PasswordKey::rawKey() const
{
    return QByteArray::fromRawData(m_key, SHA256_SIZE);
}

void PasswordKey::setPassword(const QString& password)
{
    std::memcpy(m_key, CryptoHash::hash(password.toUtf8(), CryptoHash::Sha256).data(), SHA256_SIZE);
}
