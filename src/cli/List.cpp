/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

const QCommandLineOption List::RecursiveOption =
    QCommandLineOption(QStringList() << "R"
                                     << "recursive",
                       QObject::tr("Recursively list the elements of the group."));

const QCommandLineOption List::FlattenOption = QCommandLineOption(QStringList() << "f"
                                                                                << "flatten",
                                                                  QObject::tr("Flattens the output to single lines."));

List::List()
{
    name = QString("ls");
    description = QObject::tr("List database entries.");
    options.append(List::RecursiveOption);
    options.append(List::FlattenOption);
    optionalArguments.append(
        {QString("group"), QObject::tr("Path of the group to list. Default is /"), QString("[group]")});
}

int List::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    bool recursive = parser->isSet(List::RecursiveOption);
    bool flatten = parser->isSet(List::FlattenOption);

    // No group provided, defaulting to root group.
    if (args.size() == 1) {
        out << database->rootGroup()->print(recursive, flatten) << flush;
        return EXIT_SUCCESS;
    }

    const QString& groupPath = args.at(1);
    Group* group = database->rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        err << QObject::tr("Cannot find group %1.").arg(groupPath) << endl;
        return EXIT_FAILURE;
    }

    out << group->print(recursive, flatten) << flush;
    return EXIT_SUCCESS;
}
