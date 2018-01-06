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

#ifndef KEEPASSX_KDBX3WRITER_H
#define KEEPASSX_KDBX3WRITER_H

#include <QCoreApplication>

#include "format/KeePass2.h"
#include "format/KeePass2Writer.h"
#include "keys/CompositeKey.h"

class Database;
class QIODevice;

class Kdbx3Writer: public BaseKeePass2Writer
{
Q_DECLARE_TR_FUNCTIONS(Kdbx3Writer)

public:
    Kdbx3Writer();

    using BaseKeePass2Writer::writeDatabase;
    bool writeDatabase(QIODevice* device, Database* db);

private:
    bool writeData(const QByteArray& data);
    bool writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data);

    QIODevice* m_device;
};

#endif // KEEPASSX_KDBX3WRITER_H
