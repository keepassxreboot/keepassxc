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

#ifndef KEEPASSX_KDF_H
#define KEEPASSX_KDF_H

#include <QUuid>
#include <QVariant>

#include <climits>
#include <optional>

#define KDF_MIN_SEED_SIZE 8
#define KDF_MAX_SEED_SIZE 32
#define KDF_DEFAULT_ROUNDS 1000000ull

class Kdf
{
public:
    explicit Kdf(const QUuid& uuid);
    virtual ~Kdf() = default;

    const QUuid& uuid() const;

    int rounds() const;
    virtual bool setRounds(int rounds);
    QByteArray seed() const;
    virtual bool setSeed(const QByteArray& seed);
    virtual void randomizeSeed();

    /**
     * Read the KDF parameters from a KDBX4+ parameter map.
     *
     * @param p variant map containing parameters
     * @param error if not null, receives a description of why a parameter was rejected (may stay empty)
     * @return true on success
     */
    virtual bool processParameters(const QVariantMap& p, QString* error = nullptr) = 0;
    virtual QVariantMap writeParameters() = 0;
    virtual bool transform(const QByteArray& raw, QByteArray& result) const = 0;
    virtual QSharedPointer<Kdf> clone() const = 0;

    virtual QString toString() const = 0;

    virtual int benchmark(int msec) const = 0;

    /*
     * Default target encryption time, in MS.
     */
    static const int DEFAULT_ENCRYPTION_TIME = 1000;
    /*
     * Minimum target encryption time, in MS.
     */
    static const int MIN_ENCRYPTION_TIME = 100;
    /*
     * Maximum target encryption time, in MS.
     */
    static const int MAX_ENCRYPTION_TIME = 5000;
    /*
     * Maximum number of rounds, limited by the type of rounds().
     */
    static const int MAX_ROUNDS = INT_MAX - 1;

protected:
    /**
     * Read an unsigned integer from a KDBX4+ parameter map without truncating it.
     *
     * @param p variant map containing parameters
     * @param name name of the parameter
     * @return the value, or nothing if the parameter is missing, not an integer or negative
     */
    static std::optional<quint64> unsignedParameter(const QVariantMap& p, const QString& name);

    /**
     * Read the number of rounds from a KDBX4+ parameter map without truncating it, and apply it.
     *
     * @param p variant map containing parameters
     * @param name name of the parameter
     * @param message translated error for a value above MAX_ROUNDS, with %1 for the value and %2 for the maximum
     * @param error if not null, receives message if the value is above MAX_ROUNDS
     * @return true on success
     */
    bool processRoundsParameter(const QVariantMap& p, const QString& name, const QString& message, QString* error);

    int m_rounds;
    QByteArray m_seed;

private:
    const QUuid m_uuid;
};
#endif // KEEPASSX_KDF_H
