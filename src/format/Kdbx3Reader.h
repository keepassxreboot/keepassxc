/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_KDBX3READER_H
#define KEEPASSX_KDBX3READER_H

#include "format/KdbxReader.h"

/**
 * KDBX 2/3 reader implementation.
 */
class Kdbx3Reader: public KdbxReader
{
Q_DECLARE_TR_FUNCTIONS(Kdbx3Reader)

public:
    Database* readDatabaseImpl(QIODevice* device, const QByteArray& headerData,
                               const CompositeKey& key, bool keepDatabase) override;

protected:
    bool readHeaderField(StoreDataStream& headerStream) override;
};

#endif // KEEPASSX_KDBX3READER_H
