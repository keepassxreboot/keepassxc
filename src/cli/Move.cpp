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

#include "Move.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

Move::Move()
{
    name = QString("mv");
    description = QObject::tr("Moves an entry to a new group.");
    positionalArguments.append({QString("entry"), QObject::tr("Path of the entry to move."), QString("")});
    positionalArguments.append({QString("group"), QObject::tr("Path of the destination group."), QString("")});
}

Move::~Move()
{
}

int Move::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    const QStringList args = parser->positionalArguments();
    const QString& entryPath = args.at(1);
    const QString& destinationPath = args.at(2);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        errorTextStream << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    Group* destinationGroup = database->rootGroup()->findGroupByPath(destinationPath);
    if (!destinationGroup) {
        errorTextStream << QObject::tr("Could not find group with path %1.").arg(destinationPath) << endl;
        return EXIT_FAILURE;
    }

    if (destinationGroup == entry->parent()) {
        errorTextStream << QObject::tr("Entry is already in group %1.").arg(destinationPath) << endl;
        return EXIT_FAILURE;
    }

    entry->beginUpdate();
    entry->setGroup(destinationGroup);
    entry->endUpdate();

    QString errorMessage;
    if (!database->save(&errorMessage, true, false)) {
        errorTextStream << QObject::tr("Writing the database failed %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    outputTextStream << QObject::tr("Successfully moved entry %1 to group %2.").arg(entry->title(), destinationPath)
                     << endl;
    return EXIT_SUCCESS;
}
