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

#include <cstdlib>
#include <stdio.h>

#include "Locate.h"

#include <QCommandLineParser>
#include <QStringList>

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"

Locate::Locate()
{
    name = QString("locate");
    description = QObject::tr("Find entries quickly.");
}

Locate::~Locate()
{
}

int Locate::execute(const QStringList& arguments)
{
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addPositionalArgument("term", QObject::tr("Search term."));
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        errorTextStream << parser.helpText().replace("keepassxc-cli", "keepassxc-cli locate");
        return EXIT_FAILURE;
    }

    auto db = Utils::unlockDatabase(args.at(0),
                                    parser.value(Command::KeyFileOption),
                                    parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    return locateEntry(db.data(), args.at(1));
}

int Locate::locateEntry(Database* database, const QString& searchTerm)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QStringList results = database->rootGroup()->locate(searchTerm);
    if (results.isEmpty()) {
        errorTextStream << "No results for that search term." << endl;
        return EXIT_FAILURE;
    }

    for (const QString& result : asConst(results)) {
        outputTextStream << result << endl;
    }
    return EXIT_SUCCESS;
}
