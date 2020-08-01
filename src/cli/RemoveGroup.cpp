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


CommandArgs RemoveGroup::getParserArgs(const CommandCtx& ctx) const
{
    static const CommandArgs args {
        { {"group", QObject::tr("Path of the group to remove."), ""} },
        {},
        {}
    };
    return DatabaseCommand::getParserArgs(ctx).merge(args);
}

int RemoveGroup::executeWithDatabase(CommandCtx& ctx, const QCommandLineParser& parser)
{
    auto& out = parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    // Recursive option means were looking for a group to remove.
    Database& database = ctx.getDb();
    const QString& groupPath = getArg(0, ctx.getRunmode(), parser.positionalArguments());
    const QPointer<Group> group = database.rootGroup()->findGroupByPath(groupPath);
    if (!group) {
        err << QObject::tr("Group %1 not found.").arg(groupPath) << endl;
        return EXIT_FAILURE;
    }

    if (group == database.rootGroup()) {
        err << QObject::tr("Cannot remove root group from database.") << endl;
        return EXIT_FAILURE;
    }

    bool recycled = true;
    auto* recycleBin = database.metadata()->recycleBin();
    if (!database.metadata()->recycleBinEnabled() || (recycleBin && recycleBin->findGroupByUuid(group->uuid()))) {
        delete group;
        recycled = false;
    } else {
        database.recycleGroup(group);
    };

    QString errorMessage;
    if (!database.save(&errorMessage, true, false)) {
        err << QObject::tr("Unable to save database to file: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (recycled) {
        out << QObject::tr("Successfully recycled group %1.").arg(groupPath) << endl;
    } else {
        out << QObject::tr("Successfully deleted group %1.").arg(groupPath) << endl;
    }

    return EXIT_SUCCESS;
}
