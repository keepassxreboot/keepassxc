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

#include "RemoveGroup.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"

RemoveGroup::RemoveGroup()
{
    name = QString("rmdir");
    description = QString("Removes a group from a database.");
    positionalArguments.append({QString("group"), QObject::tr("Path of the group to remove."), QString("")});
}

RemoveGroup::~RemoveGroup()
{
}

int RemoveGroup::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    bool quiet = parser->isSet(Command::QuietOption);
    QString groupPath = parser->positionalArguments().at(1);

    TextStream outputTextStream(quiet ? Utils::DEVNULL : Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    // Recursive option means were looking for a group to remove.
    QPointer<Group> group = database->rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        errorTextStream << QObject::tr("Group %1 not found.").arg(groupPath) << endl;
        return EXIT_FAILURE;
    }

    if (group == database->rootGroup()) {
        errorTextStream << QObject::tr("Cannot remove root group from database.") << endl;
        return EXIT_FAILURE;
    }

    bool recycled = true;
    auto* recycleBin = database->metadata()->recycleBin();
    if (!database->metadata()->recycleBinEnabled() || (recycleBin && recycleBin->findGroupByUuid(group->uuid()))) {
        delete group;
        recycled = false;
    } else {
        database->recycleGroup(group);
    };

    QString errorMessage;
    if (!database->save(&errorMessage, true, false)) {
        errorTextStream << QObject::tr("Unable to save database to file: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (recycled) {
        outputTextStream << QObject::tr("Successfully recycled group %1.").arg(groupPath) << endl;
    } else {
        outputTextStream << QObject::tr("Successfully deleted group %1.").arg(groupPath) << endl;
    }

    return EXIT_SUCCESS;
}
