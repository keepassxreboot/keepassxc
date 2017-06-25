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

#ifndef KEEPASSX_KEEPASS2_H
#define KEEPASSX_KEEPASS2_H

#include <QtGlobal>

#include "core/Uuid.h"

namespace KeePass2
{
    const quint32 SIGNATURE_1 = 0x9AA2D903;
    const quint32 SIGNATURE_2 = 0xB54BFB67;
    const quint32 FILE_VERSION = 0x00030001;
    const quint32 FILE_VERSION_MIN = 0x00020000;
    const quint32 FILE_VERSION_CRITICAL_MASK = 0xFFFF0000;

    const QSysInfo::Endian BYTEORDER = QSysInfo::LittleEndian;

    const Uuid CIPHER_AES = Uuid(QByteArray::fromHex("31c1f2e6bf714350be5805216afc5aff"));
    const Uuid CIPHER_TWOFISH = Uuid(QByteArray::fromHex("ad68f29f576f4bb9a36ad47af965346c"));

    const QByteArray INNER_STREAM_SALSA20_IV("\xE8\x30\x09\x4B\x97\x20\x5D\x2A");

    enum HeaderFieldID
    {
        EndOfHeader = 0,
        Comment = 1,
        CipherID = 2,
        CompressionFlags = 3,
        MasterSeed = 4,
        TransformSeed = 5,
        TransformRounds = 6,
        EncryptionIV = 7,
        ProtectedStreamKey = 8,
        StreamStartBytes = 9,
        InnerRandomStreamID = 10
    };

    enum ProtectedStreamAlgo
    {
        ArcFourVariant = 1,
        Salsa20 = 2
    };
}

#endif // KEEPASSX_KEEPASS2_H
