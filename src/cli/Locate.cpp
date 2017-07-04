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

#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>

#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "gui/UnlockDatabaseDialog.h"

Locate::Locate()
{
    this->name = QString("locate");
    this->description = QString("Find entries quickly.");
}

Locate::~Locate()
{
}

int Locate::execute(int argc, char** argv)
{

    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Find entries quickly."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    QCommandLineOption guiPrompt(QStringList() << "g"
                                               << "gui-prompt",
                                 QCoreApplication::translate("main", "Use a GUI prompt unlocking the database."));
    parser.addOption(guiPrompt);
    parser.addPositionalArgument("term", QCoreApplication::translate("main", "Search term."));
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        QCoreApplication app(argc, argv);
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = nullptr;
    QApplication app(argc, argv);
    if (parser.isSet("gui-prompt")) {
        db = UnlockDatabaseDialog::openDatabasePrompt(args.at(0));
    } else {
        db = Database::unlockFromStdin(args.at(0));
    }

    if (!db) {
        return EXIT_FAILURE;
    }
    return this->locateEntry(db, args.at(1));
}

int Locate::locateEntry(Database* database, QString searchTerm)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    QStringList results = database->rootGroup()->locate(searchTerm);
    if (results.isEmpty()) {
        outputTextStream << "No results for that search term" << endl;
        return EXIT_SUCCESS;
    }

    for (QString result : results) {
        outputTextStream << result << endl;
    }
    return EXIT_SUCCESS;
}
