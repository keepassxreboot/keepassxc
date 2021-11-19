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

#include "format/KeePass2Reader.h"
#include "format/Kdbx3Reader.h"
#include "format/Kdbx4Reader.h"
#include "format/KeePass1.h"
#include "keys/CompositeKey.h"

#include <QFile>

/**
 * Read database from file and detect correct file format.
 *
 * @param filename input file
 * @param key database encryption composite key
 * @param db Database to read into
 * @return true on success
 */
bool KeePass2Reader::readDatabase(const QString& filename, QSharedPointer<const CompositeKey> key, Database* db)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        raiseError(file.errorString());
        return false;
    }

    bool ok = readDatabase(&file, std::move(key), db);

    if (file.error() != QFile::NoError) {
        raiseError(file.errorString());
        return false;
    }

    return ok;
}

/**
 * Read database from device and detect correct file format.
 *
 * @param device input device
 * @param key database encryption composite key
 * @param db Database to read into
 * @return true on success
 */
bool KeePass2Reader::readDatabase(QIODevice* device, QSharedPointer<const CompositeKey> key, Database* db)
{
    m_error = false;
    m_errorStr.clear();

    quint32 signature1, signature2;
    bool ok = KdbxReader::readMagicNumbers(device, signature1, signature2, m_version);

    if (!ok) {
        raiseError(tr("Failed to read database file."));
        return false;
    }

    if (signature1 == KeePass1::SIGNATURE_1 && signature2 == KeePass1::SIGNATURE_2) {
        raiseError(tr("The selected file is an old KeePass 1 database (.kdb).\n\n"
                      "You can import it by clicking on Database > 'Import KeePass 1 database…'.\n"
                      "This is a one-way migration. You won't be able to open the imported "
                      "database with the old KeePassX 0.4 version."));
        return false;
    } else if (!(signature1 == KeePass2::SIGNATURE_1 && signature2 == KeePass2::SIGNATURE_2)) {
        raiseError(tr("Not a KeePass database."));
        return false;
    }

    if (m_version < KeePass2::FILE_VERSION_MIN
        || (m_version & KeePass2::FILE_VERSION_CRITICAL_MASK) > KeePass2::FILE_VERSION_MAX) {
        raiseError(tr("Unsupported KeePass 2 database version."));
        return false;
    }

    // determine file format (KDBX 2/3 or 4)
    if (m_version < KeePass2::FILE_VERSION_4) {
        m_reader.reset(new Kdbx3Reader());
    } else {
        m_reader.reset(new Kdbx4Reader());
    }

    return m_reader->readDatabase(device, std::move(key), db);
}

bool KeePass2Reader::hasError() const
{
    return m_error || (!m_reader.isNull() && m_reader->hasError());
}

QString KeePass2Reader::errorString() const
{
    return !m_reader.isNull() ? m_reader->errorString() : m_errorStr;
}

/**
 * @return detected KDBX version
 */
quint32 KeePass2Reader::version() const
{
    return m_version;
}

/**
 * @return KDBX reader used for reading the input file
 */
QSharedPointer<KdbxReader> KeePass2Reader::reader() const
{
    return m_reader;
}

/**
 * Raise an error. Use in case of an unexpected read error.
 *
 * @param errorMessage error message
 */
void KeePass2Reader::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}
