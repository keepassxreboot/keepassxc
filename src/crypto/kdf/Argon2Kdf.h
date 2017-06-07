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

#ifndef KEEPASSX_ARGON2KDF_H
#define KEEPASSX_ARGON2KDF_H

#include <argon2.h>
#include <QList>

#include "Kdf.h"

class Argon2Kdf : public Kdf {
public:
    Argon2Kdf();

    bool transform(const QByteArray& raw, QByteArray& result) const override;
    void randomizeSalt() override;
    Kdf::Type type() const override;
    Kdf* clone() const override;

    const QList<Field> fields() const override;
    quint64 field(quint32 id) const override;
    bool setField(quint32 id, quint64 val) override;

    quint32 version() const;
    quint64 timeCost() const;
    quint64 memoryCost() const;
    quint32 lanes() const;
    QByteArray salt() const;
    QByteArray secret() const;
    QByteArray assocData() const;

    bool setVersion(quint32 version);
    bool setTimeCost(quint64 cost);
    bool setMemoryCost(quint64 cost);
    bool setLanes(quint32 lanes);
    bool setSalt(const QByteArray& salt);
    bool setSecret(const QByteArray& secret);
    bool setAssocData(const QByteArray& ad);

    enum class Fields : quint32
    {
        VERSION,
        TIME_COST,
        MEMORY_COST,
        LANES,
        SALT,
        SECRET,
        ASSOCIATED_DATA
    };

    static const quint32 MIN_VERSION = ARGON2_VERSION_10;
    static const quint32 MAX_VERSION = ARGON2_VERSION_13;
    static const qint32 MIN_SALT = ARGON2_MIN_SALT_LENGTH;
    static const qint32 MAX_SALT = 2147483647; // following KeePass
    static const quint64 MIN_TIME = ARGON2_MIN_TIME;
    static const quint64 MAX_TIME = ARGON2_MAX_TIME;
    static const quint64 MIN_MEMORY = ARGON2_MIN_MEMORY * 1024;
    static const quint64 MAX_MEMORY = ARGON2_MAX_MEMORY * 1024;
    static const quint32 MIN_LANES = ARGON2_MIN_LANES;
    static const quint32 MAX_LANES = ARGON2_MAX_LANES;
    static const quint64 DEFAULT_TIME = 2;
    static const quint64 DEFAULT_MEMORY = 1024 * 1024;
    static const quint32 DEFAULT_LANES = 2;

    static const QList<Kdf::Field> FIELDS;

protected:
    int benchmarkImpl(int msec) const override;

private:
    quint32 m_version;
    quint64 m_timeCost;
    quint64 m_memoryCost;
    quint32 m_lanes;
    QByteArray m_salt;
    QByteArray m_secret;
    QByteArray m_assocData;

    static QList<Kdf::Field> initFields();
};

#endif // KEEPASSX_ARGON2KDF_H
