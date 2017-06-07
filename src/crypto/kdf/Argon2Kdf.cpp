/*
 *  Copyright (C) 2017 KeePassXC Team
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

#include <argon2.h>
#include <QThread>
#include <QElapsedTimer>

#include "format/KeePass2.h"
#include "crypto/Random.h"
#include "Argon2Kdf.h"

const QList<Kdf::Field> Argon2Kdf::FIELDS = Argon2Kdf::initFields();

QList<Kdf::Field> Argon2Kdf::initFields()
{
    return QList<Kdf::Field> {
            Kdf::Field(static_cast<quint32>(Fields::LANES), "Lanes", MIN_LANES, MAX_LANES, false),
            Kdf::Field(static_cast<quint32>(Fields::MEMORY_COST), "Memory cost", MIN_MEMORY, MAX_MEMORY, false),
            Kdf::Field(static_cast<quint32>(Fields::TIME_COST), "Time cost", MIN_TIME, MAX_TIME, true),
    };
}

Argon2Kdf::Argon2Kdf()
        : m_version(MAX_VERSION)
        , m_timeCost(DEFAULT_TIME)
        , m_memoryCost(DEFAULT_MEMORY)
        , m_lanes(DEFAULT_LANES)
        , m_salt(QByteArray(32, 0))
{
}

bool Argon2Kdf::transform(const QByteArray& raw, QByteArray& result) const
{
    result.resize(32);

    argon2_context a2ctx = {};

    a2ctx.threads = static_cast<quint32>(QThread::idealThreadCount());
    if (a2ctx.threads > m_lanes) {
        a2ctx.threads = m_lanes; // avoid libargon2 bug
    }

    a2ctx.lanes = m_lanes;
    a2ctx.m_cost = static_cast<quint32>(m_memoryCost / 1024);
    a2ctx.t_cost = static_cast<quint32>(m_timeCost);
    a2ctx.version = m_version;

    // FIXME libargon2 isn't const-correct
    a2ctx.salt = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_salt.data()));
    a2ctx.saltlen = static_cast<quint32>(m_salt.size());

    a2ctx.secret = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_secret.data()));
    a2ctx.secretlen = static_cast<quint32>(m_secret.size());

    a2ctx.ad = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_assocData.data()));
    a2ctx.adlen = static_cast<quint32>(m_assocData.size());

    a2ctx.pwd = const_cast<quint8*>(reinterpret_cast<const quint8*>(raw.data()));
    a2ctx.pwdlen = static_cast<quint32>(raw.size());

    a2ctx.out = const_cast<quint8*>(reinterpret_cast<const quint8*>(result.data()));
    a2ctx.outlen = static_cast<quint32>(result.size());

    a2ctx.flags = 0;

    int libResult = argon2d_ctx(&a2ctx);
    if (libResult != ARGON2_OK) {
        qWarning("Argon2 error: %s", argon2_error_message(libResult));
        return false;
    }

    return true;
}

quint32 Argon2Kdf::version() const
{
    return m_version;
}

quint64 Argon2Kdf::timeCost() const
{
    return m_timeCost;
}

quint64 Argon2Kdf::memoryCost() const
{
    return m_memoryCost;
}

quint32 Argon2Kdf::lanes() const
{
    return m_lanes;
}

QByteArray Argon2Kdf::salt() const
{
    return m_salt;
}

QByteArray Argon2Kdf::secret() const
{
    return m_secret;
}

QByteArray Argon2Kdf::assocData() const
{
    return m_assocData;
}

bool Argon2Kdf::setVersion(quint32 version)
{
    if (version < Argon2Kdf::MIN_VERSION || version > Argon2Kdf::MAX_VERSION) {
        return false;
    }

    m_version = version;
    return true;
}

bool Argon2Kdf::setTimeCost(quint64 cost)
{
    if (cost < Argon2Kdf::MIN_TIME || cost > Argon2Kdf::MAX_TIME) {
        return false;
    }

    m_timeCost = cost;
    return true;
}

bool Argon2Kdf::setMemoryCost(quint64 cost)
{
    if (cost < Argon2Kdf::MIN_MEMORY || cost > Argon2Kdf::MAX_MEMORY) {
        return false;
    }

    m_memoryCost = cost;
    return true;
}

bool Argon2Kdf::setLanes(quint32 lanes)
{
    if (lanes < Argon2Kdf::MIN_LANES || lanes > Argon2Kdf::MAX_LANES) {
        return false;
    }

    m_lanes = lanes;
    return true;
}

bool Argon2Kdf::setSalt(const QByteArray& salt)
{
    if (salt.size() < Argon2Kdf::MIN_SALT || salt.size() > Argon2Kdf::MAX_SALT) {
        return false;
    }

    m_salt = salt;
    return true;
}

bool Argon2Kdf::setSecret(const QByteArray& secret)
{
    m_secret = secret;
    return true;
}

bool Argon2Kdf::setAssocData(const QByteArray& ad)
{
    m_assocData = ad;
    return true;
}

void Argon2Kdf::randomizeSalt()
{
    setSalt(randomGen()->randomArray(32));
}

Kdf::Type Argon2Kdf::type() const
{
    return Kdf::Type::ARGON2;
}

Kdf* Argon2Kdf::clone() const
{
    return static_cast<Kdf*>(new Argon2Kdf(*this));
}

const QList<Kdf::Field> Argon2Kdf::fields() const
{
    return FIELDS;
}

quint64 Argon2Kdf::field(quint32 id) const
{
    switch (static_cast<Fields>(id)) {
        case Fields::LANES:
            return m_lanes;
        case Fields::MEMORY_COST:
            return m_memoryCost;
        case Fields::TIME_COST:
            return m_timeCost;
        default:
            return 0;
    }
}

bool Argon2Kdf::setField(quint32 id, quint64 val)
{
    switch (static_cast<Fields>(id)) {
        case Fields::LANES:
            return setLanes(static_cast<quint32>(val));
        case Fields::MEMORY_COST:
            return setMemoryCost(static_cast<quint64>(val));
        case Fields::TIME_COST:
            return setTimeCost(static_cast<quint64>(val));
        default:
            return false;
    }
}

int Argon2Kdf::benchmarkImpl(int msec) const
{
    QByteArray result(32, 0);
    QByteArray raw(32, '\x7E');

    argon2_context a2ctx = {};
    a2ctx.threads = 1;
    a2ctx.lanes = 1;
    a2ctx.t_cost = 100;
    a2ctx.m_cost = static_cast<quint32>(m_memoryCost / 1024);
    a2ctx.version = m_version;
    a2ctx.salt = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_salt.data()));
    a2ctx.saltlen = static_cast<quint32>(m_salt.size());
    a2ctx.secret = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_secret.data()));
    a2ctx.secretlen = static_cast<quint32>(m_secret.size());
    a2ctx.ad = const_cast<quint8*>(reinterpret_cast<const quint8*>(m_assocData.data()));
    a2ctx.adlen = static_cast<quint32>(m_assocData.size());
    a2ctx.pwd = const_cast<quint8*>(reinterpret_cast<const quint8*>(raw.data()));
    a2ctx.pwdlen = static_cast<quint32>(raw.size());
    a2ctx.out = const_cast<quint8*>(reinterpret_cast<const quint8*>(result.data()));
    a2ctx.outlen = static_cast<quint32>(result.size());
    a2ctx.flags = 0;

    QElapsedTimer t;
    int rounds = 0;
    t.start();
    do {
        argon2d_ctx(&a2ctx);
        rounds += 100;
    } while (!t.hasExpired(msec));

    return rounds;
}