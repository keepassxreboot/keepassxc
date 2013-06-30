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

#ifndef KEEPASSX_KEEPASS2WRITER_H
#define KEEPASSX_KEEPASS2WRITER_H

#include "format/KeePass2.h"
#include "keys/CompositeKey.h"

class Database;
class QIODevice;

class KeePass2Writer
{
public:
    KeePass2Writer();
    void writeDatabase(QIODevice* device, Database* db);
    void writeDatabase(const QString& filename, Database* db);
    bool hasError();
    QString errorString();

private:
    bool writeData(const QByteArray& data);
    bool writeHeaderField(KeePass2::HeaderFieldID fieldId, const QByteArray& data);
    void raiseError(const QString& errorMessage);

    QIODevice* m_device;
    bool m_error;
    QString m_errorStr;
};

#endif // KEEPASSX_KEEPASS2WRITER_H
