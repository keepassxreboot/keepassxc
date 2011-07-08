/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Random.h"

#include <gcrypt.h>

void Random::randomize(QByteArray& ba)
{
    randomize(ba.data(), ba.size());
}

QByteArray Random::randomArray(int len)
{
    QByteArray ba;
    ba.resize(len);

    randomize(ba);

    return ba;
}

quint32 Random::randomUInt(quint32 limit)
{
    quint32 rand;
    randomize(&rand, 4);
    return (rand % limit);
}

quint32 Random::randomUIntRange(quint32 min, quint32 max)
{
    return min + randomUInt(max - min);
}

void Random::randomize(void* data, int len)
{
    gcry_randomize(data, len, GCRY_STRONG_RANDOM);
}

Random::Random()
{
}
