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
    TextStream out(Utils::STDOUT);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addPositionalArgument("group", QObject::tr("Path of the group to list. Default is /"), "[group]");
    QCommandLineOption keyFile(QStringList() << "k" << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    QCommandLineOption recursiveOption(QStringList() << "R" << "recursive",
                                       QObject::tr("Recursive mode, list elements recursively"));
    parser.addOption(recursiveOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1 && args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli ls");
        return EXIT_FAILURE;
    }

    bool recursive = parser.isSet(recursiveOption);

    QScopedPointer<Database> db(Database::unlockFromStdin(args.at(0), parser.value(keyFile), Utils::STDOUT, Utils::STDERR));
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
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    if (groupPath.isEmpty()) {
        out << database->rootGroup()->print(recursive) << flush;
        return EXIT_SUCCESS;
    }

    Group* group = database->rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        err << QObject::tr("Cannot find group %1.").arg(groupPath) << endl;
        return EXIT_FAILURE;
    }

    out << group->print(recursive) << flush;
    return EXIT_SUCCESS;
}
