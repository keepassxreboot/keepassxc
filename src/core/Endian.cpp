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

#include "Endian.h"

#include <QtEndian>
#include <QIODevice>

namespace Endian {

qint16 bytesToInt16(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    Q_ASSERT(ba.size() == 2);

    if (byteOrder == QSysInfo::LittleEndian) {
        return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(ba.constData()));
    }
    else {
        return qFromBigEndian<qint16>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

qint32 bytesToInt32(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    Q_ASSERT(ba.size() == 4);

    if (byteOrder == QSysInfo::LittleEndian) {
        return qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(ba.constData()));
    }
    else {
        return qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

qint64 bytesToInt64(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    Q_ASSERT(ba.size() == 8);

    if (byteOrder == QSysInfo::LittleEndian) {
        return qFromLittleEndian<qint64>(reinterpret_cast<const uchar*>(ba.constData()));
    }
    else {
        return qFromBigEndian<qint64>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

quint16 bytesToUInt16(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    return static_cast<quint16>(bytesToInt16(ba, byteOrder));
}

quint32 bytesToUInt32(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    return static_cast<quint32>(bytesToInt32(ba, byteOrder));
}

quint64 bytesToUInt64(const QByteArray& ba, QSysInfo::Endian byteOrder)
{
    return static_cast<quint64>(bytesToInt64(ba, byteOrder));
}

qint16 readInt16(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    QByteArray ba = device->read(2);

    if (ba.size() != 2) {
        *ok = false;
        return 0;
    }
    else {
        *ok = true;
        return bytesToInt16(ba, byteOrder);
    }
}

qint32 readInt32(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    QByteArray ba = device->read(4);

    if (ba.size() != 4) {
        *ok = false;
        return 0;
    }
    else {
        *ok = true;
        return bytesToInt32(ba, byteOrder);
    }
}

qint64 readInt64(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    QByteArray ba = device->read(8);

    if (ba.size() != 8) {
        *ok = false;
        return 0;
    }
    else {
        *ok = true;
        return bytesToInt64(ba, byteOrder);
    }
}

quint16 readUInt16(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    return static_cast<quint16>(readInt16(device, byteOrder, ok));
}

quint32 readUInt32(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    return static_cast<quint32>(readInt32(device, byteOrder, ok));
}

quint64 readUInt64(QIODevice* device, QSysInfo::Endian byteOrder, bool* ok)
{
    return static_cast<quint64>(readInt64(device, byteOrder, ok));
}

QByteArray int16ToBytes(qint16 num, QSysInfo::Endian byteOrder)
{
    QByteArray ba;
    ba.resize(2);

    if (byteOrder == QSysInfo::LittleEndian) {
        qToLittleEndian<qint16>(num, reinterpret_cast<uchar*>(ba.data()));
    }
    else {
        qToBigEndian<qint64>(num, reinterpret_cast<uchar*>(ba.data()));
    }

    return ba;
}

QByteArray int32ToBytes(qint32 num, QSysInfo::Endian byteOrder)
{
    QByteArray ba;
    ba.resize(4);

    if (byteOrder == QSysInfo::LittleEndian) {
        qToLittleEndian<qint32>(num, reinterpret_cast<uchar*>(ba.data()));
    }
    else {
        qToBigEndian<qint32>(num, reinterpret_cast<uchar*>(ba.data()));
    }

    return ba;
}

QByteArray int64ToBytes(qint64 num, QSysInfo::Endian byteOrder)
{
    QByteArray ba;
    ba.resize(8);

    if (byteOrder == QSysInfo::LittleEndian) {
        qToLittleEndian<qint64>(num, reinterpret_cast<uchar*>(ba.data()));
    }
    else {
        qToBigEndian<qint64>(num, reinterpret_cast<uchar*>(ba.data()));
    }

    return ba;
}

bool writeInt16(qint16 num, QIODevice* device, QSysInfo::Endian byteOrder)
{
    QByteArray ba = int16ToBytes(num, byteOrder);
    int bytesWritten = device->write(ba);
    return (bytesWritten == ba.size());
}

bool writeInt32(qint32 num, QIODevice* device, QSysInfo::Endian byteOrder)
{
    QByteArray ba = int32ToBytes(num, byteOrder);
    int bytesWritten = device->write(ba);
    return (bytesWritten == ba.size());
}

bool writeInt64(qint64 num, QIODevice* device, QSysInfo::Endian byteOrder)
{
    QByteArray ba = int64ToBytes(num, byteOrder);
    int bytesWritten = device->write(ba);
    return (bytesWritten == ba.size());
}

} // namespace Endian
