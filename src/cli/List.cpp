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

#include "List.h"
#include "cli/Utils.h"

#include <QCommandLineParser>

#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

List::List()
{
    name = QString("ls");
    description = QObject::tr("List database entries.");
}

List::~List()
{
}

int List::execute(const QStringList& arguments)
{
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addPositionalArgument("group", QObject::tr("Path of the group to list. Default is /"), "[group]");
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);

    QCommandLineOption recursiveOption(QStringList() << "R"
                                                     << "recursive",
                                       QObject::tr("Recursively list the elements of the group."));
    parser.addOption(recursiveOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1 && args.size() != 2) {
        errorTextStream << parser.helpText().replace("keepassxc-cli", "keepassxc-cli ls");
        return EXIT_FAILURE;
    }

    bool recursive = parser.isSet(recursiveOption);

    auto db = Utils::unlockDatabase(args.at(0),
                                    parser.value(Command::KeyFileOption),
                                    parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    if (args.size() == 2) {
        return listGroup(db.data(), recursive, args.at(1));
    }
    return listGroup(db.data(), recursive);
}

int List::listGroup(Database* database, bool recursive, const QString& groupPath)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    if (groupPath.isEmpty()) {
        outputTextStream << database->rootGroup()->print(recursive) << flush;
        return EXIT_SUCCESS;
    }

    Group* group = database->rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        errorTextStream << QObject::tr("Cannot find group %1.").arg(groupPath) << endl;
        return EXIT_FAILURE;
    }

    outputTextStream << group->print(recursive) << flush;
    return EXIT_SUCCESS;
}
