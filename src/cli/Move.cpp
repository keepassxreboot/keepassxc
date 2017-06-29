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

#include "Move.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Group.h"
#include "cli/Utils.h"

Move::Move()
{
    this->name = QString("mv");
    this->shellUsage = QString("mv entry_path|group_path destination_group_path");
    this->description = QString("Move an entry or a group in the database.");
}

Move::~Move()
{
}

int Move::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Move an entry or a group in the database."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("group", QCoreApplication::translate("main", "Path of the group or entry to move."));
    parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Path of the destination group."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 3) {
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->move(db, args.at(0), args.at(1), args.at(2));

}

int Move::executeFromShell(Database* database, QString databasePath, QStringList arguments)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    if (arguments.size() != 2) {
        outputTextStream << this->getShellUsageLine() << endl;
        return EXIT_FAILURE;
    }
    return this->move(database, databasePath, arguments.at(0), arguments.at(1));
}

int Move::move(Database* database, QString databasePath, QString sourcePath, QString destinationPath)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    Group* destinationGroup = database->rootGroup()->findGroupByPath(destinationPath);
    if (!destinationGroup) {
        qCritical("Destination group %s not found.", qPrintable(destinationPath));
        return EXIT_FAILURE;
    }

    Entry* entry = database->rootGroup()->findEntryByPath(sourcePath);
    if (!entry) {
        Group* group = database->rootGroup()->findGroupByPath(sourcePath);
        if (group == nullptr) {
            qCritical("No group or entry found with path %s.", qPrintable(sourcePath));
            return EXIT_FAILURE;
        }
        group->setParent(destinationGroup);
    } else {
        entry->setGroup(destinationGroup);
    }

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    if (entry) {
        outputTextStream << "Successfully moved entry!\n";
    } else {
        outputTextStream << "Successfully moved group!\n";
    }
    outputTextStream.flush();

    return EXIT_SUCCESS;

}
