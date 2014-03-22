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

#ifndef KEEPASSX_SYMMETRICCIPHERBACKEND_H
#define KEEPASSX_SYMMETRICCIPHERBACKEND_H

#include <QByteArray>

class SymmetricCipherBackend
{
public:
    virtual ~SymmetricCipherBackend() {}
    virtual void setKey(const QByteArray& key) = 0;
    virtual void setIv(const QByteArray& iv) = 0;

    virtual QByteArray process(const QByteArray& data) = 0;
    virtual void processInPlace(QByteArray& data) = 0;
    virtual void processInPlace(QByteArray& data, quint64 rounds) = 0;

    virtual void reset() = 0;
    virtual int blockSize() const = 0;
};

#endif // KEEPASSX_SYMMETRICCIPHERBACKEND_H
