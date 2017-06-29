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

#include "AddGroup.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Group.h"
#include "cli/Utils.h"

AddGroup::AddGroup()
{
    this->name = QString("mkdir");
    this->shellUsage = QString("mkdir new_group_path");
    this->description = QString("Add a group to the database.");
}

AddGroup::~AddGroup()
{
}

int AddGroup::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Add a group to the database."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("group", QCoreApplication::translate("main", "Path of the group to add."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->addGroup(db, args.at(0), args.at(1));

}

int AddGroup::executeFromShell(Database* database, QString databasePath, QStringList arguments)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    if (arguments.size() != 1) {
        outputTextStream << this->getShellUsageLine() << endl;
        return EXIT_FAILURE;
    }
    return this->addGroup(database, databasePath, arguments.at(0));
}

int AddGroup::addGroup(Database* database, QString databasePath, QString groupPath)
{

    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    Group* group = database->rootGroup()->findGroupByPath(groupPath);
    if (group != nullptr) {
	qCritical("Group %s already exists.", qPrintable(groupPath));
	return EXIT_FAILURE;
    }

    group = database->rootGroup()->addGroupWithPath(groupPath);
    if (group == nullptr) {
        return EXIT_FAILURE;
    }

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    outputTextStream << "Successfully added new group!" << endl;

    return EXIT_SUCCESS;

}
