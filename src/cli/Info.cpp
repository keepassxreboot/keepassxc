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
#include <cstdlib>
#include <stdio.h>

#include "Info.h"
#include "Utils.h"

#include "core/Database.h"
#include "core/Global.h"
#include "core/Metadata.h"
#include "format/KeePass2.h"

Info::Info()
{
    name = QString("db-info");
    description = QObject::tr("Show a database's information.");
}

int Info::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser>)
{
    auto& out = Utils::STDOUT;

    out << QObject::tr("UUID: ") << database->uuid().toString() << endl;
    out << QObject::tr("Name: ") << database->metadata()->name() << endl;
    out << QObject::tr("Description: ") << database->metadata()->description() << endl;
    for (auto& cipher : asConst(KeePass2::CIPHERS)) {
        if (cipher.first == database->cipher()) {
            out << QObject::tr("Cipher: ") << cipher.second << endl;
        }
    }
    out << QObject::tr("KDF: ") << database->kdf()->toString() << endl;
    if (database->metadata()->recycleBinEnabled()) {
        out << QObject::tr("Recycle bin is enabled.") << endl;
    } else {
        out << QObject::tr("Recycle bin is not enabled.") << endl;
    }
    return EXIT_SUCCESS;
}
