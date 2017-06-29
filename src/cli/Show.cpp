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

#include "Show.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "cli/Utils.h"

Show::Show()
{
    this->name = QString("show");
    this->shellUsage = QString("show entry_path");
    this->description = QString("Show an entry's information.");
}

Show::~Show()
{
}

int Show::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Show a password."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("entry", QCoreApplication::translate("main", "Name of the entry to show."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->showEntry(db, args.at(1));
}

int Show::executeFromShell(Database* database, QString, QStringList arguments)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    if (arguments.size() != 1) {
        outputTextStream << this->getShellUsageLine() << endl;
        return EXIT_FAILURE;
    }
    return this->showEntry(database, arguments.at(0));
}

int Show::showEntry(Database* database, QString entryPath)
{

    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    Entry* entry = database->rootGroup()->findEntry(entryPath);
    if (!entry) {
        qCritical("Could not find entry with path %s.", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    outputTextStream << "   title: " << entry->title() << "\n";
    outputTextStream << "username: " << entry->username() << "\n";
    outputTextStream << "password: " << entry->password() << "\n";
    outputTextStream << "     URL: " << entry->url() << "\n";
    outputTextStream.flush();
    return EXIT_SUCCESS;

}
