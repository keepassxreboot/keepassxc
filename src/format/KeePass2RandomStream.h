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

#ifndef KEEPASSX_KEEPASS2RANDOMSTREAM_H
#define KEEPASSX_KEEPASS2RANDOMSTREAM_H

#include <QtCore/QByteArray>

#include "crypto/SymmetricCipher.h"

class KeePass2RandomStream
{
public:
    KeePass2RandomStream(QByteArray key);
    QByteArray randomBytes(int size);
    QByteArray process(const QByteArray& data);
    void processInPlace(QByteArray& data);

private:
    void loadBlock();

    SymmetricCipher m_cipher;
    QByteArray m_buffer;
    int m_offset;
};

#endif // KEEPASSX_KEEPASS2RANDOMSTREAM_H
