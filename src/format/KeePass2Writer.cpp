/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include <QIODevice>
#include <QString>
#include <QFile>

#include "format/KeePass2Writer.h"
#include "core/Database.h"
#include "format/Kdbx3Writer.h"
#include "format/Kdbx4Writer.h"

BaseKeePass2Writer::BaseKeePass2Writer() : m_error(false)
{
    m_errorStr.clear();
}

BaseKeePass2Writer::~BaseKeePass2Writer() {}

bool BaseKeePass2Writer::hasError()
{
    return m_error;
}

QString BaseKeePass2Writer::errorString()
{
    return m_errorStr;
}

void BaseKeePass2Writer::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}

bool BaseKeePass2Writer::writeDatabase(const QString& filename, Database* db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        raiseError(file.errorString());
        return false;
    }
    return writeDatabase(&file, db);
}

bool KeePass2Writer::hasError()
{
    return m_error || (m_writer && m_writer->hasError());
}

QString KeePass2Writer::errorString()
{
    return m_writer ? m_writer->errorString() : m_errorStr;
}

bool KeePass2Writer::writeDatabase(QIODevice* device, Database* db) {
    bool useKdbx4 = false;

    if (db->kdf()->uuid() != KeePass2::KDF_AES) {
        useKdbx4 = true;
    }

    if (db->publicCustomData().size() > 0) {
        useKdbx4 = true;
    }

    // Determine KDBX3 vs KDBX4
    if (useKdbx4) {
        m_writer.reset(new Kdbx4Writer());
    } else {
        m_writer.reset(new Kdbx3Writer());
    }

    return m_writer->writeDatabase(device, db);
}
