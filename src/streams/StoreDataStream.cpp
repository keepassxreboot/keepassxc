/*
*  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "StoreDataStream.h"

StoreDataStream::StoreDataStream(QIODevice* baseDevice)
    : LayeredStream(baseDevice)
{
}

bool StoreDataStream::open(QIODevice::OpenMode mode)
{
    bool result = LayeredStream::open(mode);

    if (result) {
        m_storedData.clear();
    }

    return result;
}

QByteArray StoreDataStream::storedData() const
{
    return m_storedData;
}

qint64 StoreDataStream::readData(char* data, qint64 maxSize)
{
    qint64 bytesRead = LayeredStream::readData(data, maxSize);
    if (bytesRead == -1) {
        setErrorString(m_baseDevice->errorString());
        return -1;
    }

    m_storedData.append(data, bytesRead);

    return bytesRead;
}
