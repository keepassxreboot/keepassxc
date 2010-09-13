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

#include "LayeredStream.h"

LayeredStream::LayeredStream(QIODevice* baseDevice)
    : QIODevice(baseDevice)
    , m_baseDevice(baseDevice)
{
    connect(baseDevice, SIGNAL(aboutToClose()), SLOT(closeStream()));
}

bool LayeredStream::isSequential() const
{
    return true;
}

QString LayeredStream::errorString() const
{
    return m_baseDevice->errorString();
}

bool LayeredStream::open(QIODevice::OpenMode mode)
{
    // filter out all other modes
    mode &= QIODevice::ReadWrite;

    if (mode == QIODevice::ReadWrite) {
        qWarning("Reading and writing at the same time is not supported.");
        return false;
    }
    else if (openMode() & mode) {
        return true;
    }
    else if (!(m_baseDevice->openMode() & mode)) {
        qWarning("Base device is not opened correctly.");
        return false;
    }
    else {
        setOpenMode(mode | QIODevice::Unbuffered);
        return true;
    }
}

qint64 LayeredStream::readData(char* data, qint64 maxSize)
{
    return m_baseDevice->read(data, maxSize);
}

qint64 LayeredStream::writeData(const char* data, qint64 maxSize)
{
    return m_baseDevice->write(data, maxSize);
}

void LayeredStream::closeStream()
{
    close();
}
