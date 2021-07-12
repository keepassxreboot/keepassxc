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

#include "crypto/CryptoHash.h"

#include <QSharedPointer>

QUuid PasswordKey::UUID("77e90411-303a-43f2-b773-853b05635ead");

constexpr int PasswordKey::SHA256_SIZE;

PasswordKey::PasswordKey()
    : Key(UUID)
    , m_key(SHA256_SIZE)
{
}

PasswordKey::PasswordKey(const QString& password)
    : Key(UUID)
    , m_key(SHA256_SIZE)
{
    setPassword(password);
}

QByteArray PasswordKey::rawKey() const
{
    if (!m_isInitialized) {
        return {};
    }
    return QByteArray(m_key.data(), m_key.size());
}

void PasswordKey::setPassword(const QString& password)
{
    setHash(CryptoHash::hash(password.toUtf8(), CryptoHash::Sha256));
}

void PasswordKey::setHash(const QByteArray& hash)
{
    Q_ASSERT(hash.size() == SHA256_SIZE);
    std::memcpy(m_key.data(), hash.data(), std::min(SHA256_SIZE, hash.size()));
    m_isInitialized = true;
}

QSharedPointer<PasswordKey> PasswordKey::fromRawKey(const QByteArray& rawKey)
{
    auto result = QSharedPointer<PasswordKey>::create();
    result->setHash(rawKey);
    return result;
}
