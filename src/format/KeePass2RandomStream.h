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

#include <QByteArray>

#include "crypto/SymmetricCipher.h"
#include "KeePass2.h"

class KeePass2RandomStream
{
public:
    KeePass2RandomStream(KeePass2::ProtectedStreamAlgo algo);

    bool init(const QByteArray& key);
    QByteArray randomBytes(int size, bool* ok);
    QByteArray process(const QByteArray& data, bool* ok);
    Q_REQUIRED_RESULT bool processInPlace(QByteArray& data);
    QString errorString() const;

private:
    bool loadBlock();

    SymmetricCipher m_cipher;
    QByteArray m_buffer;
    int m_offset;

    static SymmetricCipher::Algorithm mapAlgo(KeePass2::ProtectedStreamAlgo algo);
};

#endif // KEEPASSX_KEEPASS2RANDOMSTREAM_H
