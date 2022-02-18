/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
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

#include "ChallengeResponseKey.h"

#include "core/AsyncTask.h"

QUuid ChallengeResponseKey::UUID("e092495c-e77d-498b-84a1-05ae0d955508");

ChallengeResponseKey::ChallengeResponseKey(YubiKeySlot keySlot)
    : Key(UUID)
    , m_keySlot(keySlot)
{
}

QByteArray ChallengeResponseKey::rawKey() const
{
    return QByteArray(m_key.data(), m_key.size());
}

void ChallengeResponseKey::setRawKey(const QByteArray&)
{
    // Nothing to do here
}

YubiKeySlot ChallengeResponseKey::slotData() const
{
    return m_keySlot;
}

QString ChallengeResponseKey::error() const
{
    return m_error;
}

bool ChallengeResponseKey::challenge(const QByteArray& challenge)
{
    m_error.clear();
    auto result =
        AsyncTask::runAndWaitForFuture([&] { return YubiKey::instance()->challenge(m_keySlot, challenge, m_key); });

    if (result != YubiKey::ChallengeResult::YCR_SUCCESS) {
        // Record the error message
        m_key.clear();
        m_error = YubiKey::instance()->errorMessage();
    }

    return result == YubiKey::ChallengeResult::YCR_SUCCESS;
}

QByteArray ChallengeResponseKey::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << uuid().toRfc4122() << m_keySlot;
    return data;
}

void ChallengeResponseKey::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    QByteArray uuidData;
    stream >> uuidData;
    if (uuid().toRfc4122() == uuidData) {
        stream >> m_keySlot;
    }
}
