/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseInfo.h"

#include "Utils.h"
#include "core/DatabaseStats.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"

#include <QCommandLineParser>

DatabaseInfo::DatabaseInfo()
{
    name = QString("db-info");
    description = QObject::tr("Show a database's information.");
}

int DatabaseInfo::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser>)
{
    auto& out = Utils::STDOUT;

    out << QObject::tr("UUID: ") << database->uuid().toString() << endl;
    out << QObject::tr("Name: ") << database->metadata()->name() << endl;
    out << QObject::tr("Description: ") << database->metadata()->description() << endl;
    for (auto& cipher : asConst(KeePass2::CIPHERS)) {
        if (cipher == database->cipher()) {
            out << QObject::tr("Cipher: ") << KeePass2::cipherToString(cipher) << endl;
        }
    }
    out << QObject::tr("KDF: ") << database->kdf()->toString() << endl;
    if (database->metadata()->recycleBinEnabled()) {
        out << QObject::tr("Recycle bin is enabled.") << endl;
    } else {
        out << QObject::tr("Recycle bin is not enabled.") << endl;
    }

    DatabaseStats stats(database);
    out << QObject::tr("Location") << ": " << database->filePath() << endl;
    out << QObject::tr("Database created") << ": "
        << database->rootGroup()->timeInfo().creationTime().toString(Qt::DefaultLocaleShortDate) << endl;
    out << QObject::tr("Last saved") << ": " << stats.modified.toString(Qt::DefaultLocaleShortDate) << endl;
    out << QObject::tr("Unsaved changes") << ": " << (database->isModified() ? QObject::tr("yes") : QObject::tr("no"))
        << endl;
    out << QObject::tr("Number of groups") << ": " << QString::number(stats.groupCount) << endl;
    out << QObject::tr("Number of entries") << ": " << QString::number(stats.entryCount) << endl;
    out << QObject::tr("Number of expired entries") << ": " << QString::number(stats.expiredEntries) << endl;
    out << QObject::tr("Unique passwords") << ": " << QString::number(stats.uniquePasswords) << endl;
    out << QObject::tr("Non-unique passwords") << ": " << QString::number(stats.reusedPasswords) << endl;
    out << QObject::tr("Maximum password reuse") << ": " << QString::number(stats.maxPwdReuse()) << endl;
    out << QObject::tr("Number of short passwords") << ": " << QString::number(stats.shortPasswords) << endl;
    out << QObject::tr("Number of weak passwords") << ": " << QString::number(stats.weakPasswords) << endl;
    out << QObject::tr("Entries excluded from reports") << ": " << QString::number(stats.excludedEntries) << endl;
    out << QObject::tr("Average password length") << ": " << QObject::tr("%1 characters").arg(stats.averagePwdLength())
        << endl;

    return EXIT_SUCCESS;
}
