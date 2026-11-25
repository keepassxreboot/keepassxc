/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "Kdf.h"

#include "crypto/Random.h"

Kdf::Kdf(const QUuid& uuid)
    : m_rounds(KDF_DEFAULT_ROUNDS)
    , m_seed(QByteArray(KDF_MAX_SEED_SIZE, 0))
    , m_uuid(uuid)
{
}

const QUuid& Kdf::uuid() const
{
    return m_uuid;
}

int Kdf::rounds() const
{
    return m_rounds;
}

QByteArray Kdf::seed() const
{
    return m_seed;
}

bool Kdf::setRounds(int rounds)
{
    if (rounds >= 1 && rounds <= MAX_ROUNDS) {
        m_rounds = rounds;
        return true;
    }
    m_rounds = 1;
    return false;
}

bool Kdf::setSeed(const QByteArray& seed)
{
    if (seed.size() < KDF_MIN_SEED_SIZE || seed.size() > KDF_MAX_SEED_SIZE) {
        return false;
    }

    m_seed = seed;
    return true;
}

void Kdf::randomizeSeed()
{
    setSeed(randomGen()->randomArray(m_seed.size()));
}

std::optional<quint64> Kdf::unsignedParameter(const QVariantMap& p, const QString& name)
{
    const auto value = p.value(name);
    switch (value.typeId()) {
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return value.toULongLong();
    case QMetaType::Int:
    case QMetaType::LongLong:
        if (value.toLongLong() >= 0) {
            return value.toULongLong();
        }
        return {};
    default:
        // Other types, like strings, would be converted rather than rejected by QVariant
        return {};
    }
}

bool Kdf::processRoundsParameter(const QVariantMap& p, const QString& name, const QString& message, QString* error)
{
    const auto rounds = unsignedParameter(p, name);
    if (!rounds || *rounds > static_cast<quint64>(MAX_ROUNDS) || !setRounds(static_cast<int>(*rounds))) {
        if (rounds && error && *rounds > static_cast<quint64>(MAX_ROUNDS)) {
            *error = message.arg(*rounds).arg(MAX_ROUNDS);
        }
        return false;
    }
    return true;
}
