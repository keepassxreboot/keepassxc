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

#include <QVariant>

#include "core/Uuid.h"

class Kdf
{
public:
    explicit Kdf(Uuid uuid);
    virtual ~Kdf() = default;

    Uuid uuid() const;

    virtual quint64 rounds() const = 0;
    virtual bool setRounds(quint64 rounds) = 0;
    virtual QByteArray seed() const = 0;
    virtual bool setSeed(const QByteArray& seed) = 0;
    virtual bool transform(const QByteArray& raw, QByteArray& result) const = 0;
    virtual void randomizeTransformSalt() = 0;
    virtual QSharedPointer<Kdf> clone() const = 0;

    virtual int benchmark(int msec) const;

protected:
    virtual int benchmarkImpl(int msec) const = 0;

private:
    class BenchmarkThread;
    const Uuid m_uuid;

};

#endif // KEEPASSX_KDF_H
