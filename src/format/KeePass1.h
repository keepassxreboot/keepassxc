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

#ifndef KEEPASSX_KEEPASS1_H
#define KEEPASSX_KEEPASS1_H

#include <QtGlobal>

namespace KeePass1
{
    const quint32 SIGNATURE_1 = 0x9AA2D903;
    const quint32 SIGNATURE_2 = 0xB54BFB65;
    const quint32 PWM_DBVER_DW = 0x00030002;
    const quint32 FILE_VERSION = 0x00030002;
    const quint32 FILE_VERSION_CRITICAL_MASK = 0xFFFFFF00;

    const QSysInfo::Endian BYTEORDER = QSysInfo::LittleEndian;

    enum EncryptionFlags
    {
        Rijndael = 2,
        Twofish = 8
    };
}

#endif // KEEPASSX_KEEPASS1_H
