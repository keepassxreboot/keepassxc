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

#include "Totp.h"

#include <cstdlib>
#include <stdio.h>

#include <QCommandLineParser>

#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Global.h"
#include "Utils.h"

ShowTotp::ShowTotp()
{
    name = QString("totp");
    description = QObject::tr("Show an entry's TOTP.");
}

ShowTotp::~ShowTotp()
{
}

int ShowTotp::execute(const QStringList& arguments)
{
    TextStream out(Utils::STDOUT);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    QCommandLineOption keyFile(QStringList() << "k"  << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.addPositionalArgument("entry", QObject::tr("Name of the entry whose TOTP to show."));
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli totp");
        return EXIT_FAILURE;
    }

    QScopedPointer<Database> db(Database::unlockFromStdin(args.at(0), parser.value(keyFile), Utils::STDOUT, Utils::STDERR));
    if (!db) {
        return EXIT_FAILURE;
    }

    return showEntry(db.data(), args.at(1));
}

int ShowTotp::showEntry(Database* database, const QString& entryPath)
{
    TextStream in(Utils::STDIN, QIODevice::ReadOnly);
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    if (!entry->hasTotp()) {
        err << QObject::tr("Entry with path %1 has no TOTP set up.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    out << entry->totp() << endl;
    return EXIT_SUCCESS;
}
