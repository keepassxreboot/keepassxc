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

#ifndef KEEPASSX_KDF_H
#define KEEPASSX_KDF_H

#include <QVariant>

#include "core/Uuid.h"

class Kdf
{
public:
    enum class Type {
        AES,
        ARGON2
    };

    class Field {
    public:
        Field(quint32 id, const QString& name, quint64 min, quint64 max, bool benchmark = false);

        quint32 id() const;
        QString name() const;
        quint64 min() const;
        quint64 max() const;
        bool benchmarked() const;

    private:
        quint32 m_id;
        QString m_name;
        quint64 m_min;
        quint64 m_max;
        bool m_benchmark;
    };

    virtual ~Kdf() {}
    virtual bool transform(const QByteArray& raw, QByteArray& result) const = 0;
    virtual void randomizeSalt() = 0;
    virtual Type type() const = 0;
    virtual Kdf* clone() const = 0;

    virtual const QList<Field> fields() const = 0;
    virtual quint64 field(quint32 id) const = 0;
    virtual bool setField(quint32 id, quint64 val) = 0;
    virtual int benchmark(int msec) const;

protected:
    virtual int benchmarkImpl(int msec) const = 0;

private:
    class BenchmarkThread;

};

#endif // KEEPASSX_KDF_H
