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

#include "Help.h"

#include "Command.h"
#include "TextStream.h"
#include "Utils.h"

Help::Help()
{
    name = QString("help");
    description = QObject::tr("Display command help.");

    positionalArguments.append({QString("command"), QObject::tr("Command name."), QString("")});
}

int Help::execImpl(CommandCtx& ctx, const QCommandLineParser& parser)
{
    auto& out = Utils::STDOUT;

    const QString& cmdName = parser.positionalArguments().first();
    QSharedPointer<Command> cmd = ctx.getCmd(cmdName);
    BREAK_IF(!cmd, EXIT_FAILURE,
             ctx, QString("Command '%1' not found.").arg(cmdName));

    out << cmd->getHelpText(parser) << endl;
    return EXIT_SUCCESS;
}
