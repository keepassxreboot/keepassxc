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
    virtual bool init() = 0;
    virtual bool setKey(const QByteArray& key) = 0;
    virtual bool setIv(const QByteArray& iv) = 0;

    virtual QByteArray process(const QByteArray& data, bool* ok) = 0;
    Q_REQUIRED_RESULT virtual bool processInPlace(QByteArray& data) = 0;
    Q_REQUIRED_RESULT virtual bool processInPlace(QByteArray& data, quint64 rounds) = 0;

    virtual bool reset() = 0;
    virtual int keySize() const = 0;
    virtual int blockSize() const = 0;

    virtual QString errorString() const = 0;
};

#endif // KEEPASSX_SYMMETRICCIPHERBACKEND_H
