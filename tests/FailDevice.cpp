/*
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#include "FailDevice.h"

FailDevice::FailDevice(int failAfter, QObject* parent)
    : QBuffer(parent)
    , m_failAfter(failAfter)
    , m_readCount(0)
    , m_writeCount(0)
{
}

bool FailDevice::open(QIODevice::OpenMode openMode)
{
    return QBuffer::open(openMode | QIODevice::Unbuffered);
}

qint64 FailDevice::readData(char* data, qint64 len)
{
    if (m_readCount >= m_failAfter) {
        setErrorString("FAILDEVICE");
        return -1;
    } else {
        qint64 result = QBuffer::readData(data, len);
        if (result != -1) {
            m_readCount += result;
        }

        return result;
    }
}

qint64 FailDevice::writeData(const char* data, qint64 len)
{
    if (m_writeCount >= m_failAfter) {
        setErrorString("FAILDEVICE");
        return -1;
    } else {
        qint64 result = QBuffer::writeData(data, len);
        if (result != -1) {
            m_writeCount += result;
        }

        return result;
    }
}
