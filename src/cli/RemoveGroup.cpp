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

#include "RemoveGroup.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Tools.h"
#include "cli/Utils.h"

RemoveGroup::RemoveGroup()
{
    this->name = QString("rmdir");
    this->shellUsage = QString("rmdir group_path");
    this->description = QString("Remove a group from the database.");
}

RemoveGroup::~RemoveGroup()
{
}

int RemoveGroup::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Remove a group from the database."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("group", QCoreApplication::translate("main", "Path of the group to remove."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->removeGroup(db, args.at(0), args.at(1));

}

int RemoveGroup::executeFromShell(Database* database, QString databasePath, QStringList arguments)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    if (arguments.size() != 1) {
        outputTextStream << this->getShellUsageLine() << endl;
        return EXIT_FAILURE;
    }
    return this->removeGroup(database, databasePath, arguments.at(0));
}

int RemoveGroup::removeGroup(Database* database, QString databasePath, QString groupPath)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    Group* group = database->rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        qCritical("Group %s not found.", qPrintable(groupPath));
        return EXIT_FAILURE;
    }

    // FIXME remove this.
    Utils::createRecycleBin(database);
    QString groupName = group->name();
    bool inRecycleBin = Tools::hasChild(database->metadata()->recycleBin(), group);
    bool isRecycleBin = (group == database->metadata()->recycleBin());
    bool isRecycleBinSubgroup = Tools::hasChild(group, database->metadata()->recycleBin());
    if (inRecycleBin || isRecycleBin || isRecycleBinSubgroup || !database->metadata()->recycleBinEnabled()) {
        if (!Utils::askYesNoQuestion("You are about to remove group " + groupName + " permanently.", true)) {
            return EXIT_FAILURE;
        }
        delete group;
    } else {
        database->recycleGroup(group);
    }

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    outputTextStream << "Successfully removed group!\n";
    outputTextStream.flush();

    return EXIT_SUCCESS;

}

QStringList RemoveGroup::getSuggestions(Database* database, QStringList arguments)
{
    if (arguments.size() != 1) {
        return QStringList();
    }
    QString currentText = arguments.last();
    return database->rootGroup()->getSuggestions(arguments.at(0), false);
}
