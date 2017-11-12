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

#ifndef KEEPASSX_AESKDF_H
#define KEEPASSX_AESKDF_H

#include "Kdf.h"

class AesKdf : public Kdf
{
public:
    AesKdf();

    bool transform(const QByteArray& raw, QByteArray& result) const override;
    void randomizeTransformSalt() override;
    Kdf::Type type() const override;
    Kdf* clone() const override;

    const QList<Field> fields() const override;
    quint64 field(quint32 id) const override;
    bool setField(quint32 id, quint64 val) override;

    quint64 rounds() const;
    QByteArray seed() const;

    bool setRounds(quint64 rounds);
    bool setSeed(const QByteArray& seed);

    enum class Fields : quint32
    {
        ROUNDS,
        SEED
    };

    static const QList<Kdf::Field> FIELDS;

protected:
    int benchmarkImpl(int msec) const override;

private:
    quint64 m_rounds;
    QByteArray m_seed;

    static bool transformKeyRaw(const QByteArray& key, const QByteArray& seed, quint64 rounds, QByteArray* result) Q_REQUIRED_RESULT;
    static QList<Kdf::Field> initFields();
};

#endif // KEEPASSX_AESKDF_H
