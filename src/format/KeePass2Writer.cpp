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
#include <QFile>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/kdf/AesKdf.h"
#include "format/KeePass2Writer.h"
#include "format/Kdbx3Writer.h"
#include "format/Kdbx4Writer.h"

/**
 * Write a database to a KDBX file.
 *
 * @param filename output filename
 * @param db source database
 * @return true on success
 */
bool KeePass2Writer::writeDatabase(const QString& filename, Database* db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        raiseError(file.errorString());
        return false;
    }
    return writeDatabase(&file, db);
}

/**
 * @return true if the database should upgrade to KDBX4.
 */
bool KeePass2Writer::implicitUpgradeNeeded(Database const* db) const
{
    if (!db->publicCustomData().isEmpty()) {
        return true;
    }

    for (const auto& group: db->rootGroup()->groupsRecursive(true)) {
        if (group->customData() && !group->customData()->isEmpty()) {
            return true;
        }

        for (const auto& entry: group->entries()) {
            if (entry->customData() && !entry->customData()->isEmpty()) {
                return true;
            }

            for (const auto& historyItem: entry->historyItems()) {
                if (historyItem->customData() && !historyItem->customData()->isEmpty()) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * Write a database to a device in KDBX format.
 *
 * @param device output device
 * @param db source database
 * @return true on success
 */

bool KeePass2Writer::writeDatabase(QIODevice* device, Database* db) {
    m_error = false;
    m_errorStr.clear();

    bool upgradeNeeded = implicitUpgradeNeeded(db);
    if (upgradeNeeded) {
        // We MUST re-transform the key, because challenge-response hashing has changed in KDBX 4.
        // If we forget to re-transform, the database will be saved WITHOUT a challenge-response key component!
        db->changeKdf(KeePass2::uuidToKdf(KeePass2::KDF_AES_KDBX4));
    }

    if (db->kdf()->uuid() == KeePass2::KDF_AES_KDBX3) {
        Q_ASSERT(!upgradeNeeded);
        m_version = KeePass2::FILE_VERSION_3_1;
        m_writer.reset(new Kdbx3Writer());
    } else {
        m_version = KeePass2::FILE_VERSION_4;
        m_writer.reset(new Kdbx4Writer());
    }

    return m_writer->writeDatabase(device, db);
}

bool KeePass2Writer::hasError() const
{
    return m_error || (m_writer && m_writer->hasError());
}

QString KeePass2Writer::errorString() const
{
    return m_writer ? m_writer->errorString() : m_errorStr;
}

/**
 * Raise an error. Use in case of an unexpected write error.
 *
 * @param errorMessage error message
 */
void KeePass2Writer::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}

/**
 * @return KDBX writer used for writing the output file
 */
QSharedPointer<KdbxWriter> KeePass2Writer::writer() const
{
    return QSharedPointer<KdbxWriter>();
}

/**
 * @return KDBX version used for writing the output file
 */
quint32 KeePass2Writer::version() const
{
    return m_version;
}
