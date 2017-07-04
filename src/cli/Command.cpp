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

#include <QMap>

#include "Command.h"

#include "Add.h"
#include "Clip.h"
#include "Create.h"
#include "EntropyMeter.h"
#include "Extract.h"
#include "List.h"
#include "Locate.h"
#include "Merge.h"
#include "Remove.h"
#include "Show.h"

QMap<QString, Command*> commands;

Command::~Command()
{
}

int Command::execute(int, char**)
{
    return EXIT_FAILURE;
}

QString Command::getDescriptionLine()
{

    QString response = this->name;
    QString space(" ");
    QString spaces = space.repeated(15 - this->name.length());
    response = response.append(spaces);
    response = response.append(this->description);
    response = response.append("\n");
    return response;
}

void populateCommands()
{
    if (commands.isEmpty()) {
        commands.insert(QString("add"), new Add());
        commands.insert(QString("clip"), new Clip());
        commands.insert(QString("create"), new Create());
        commands.insert(QString("entropy-meter"), new EntropyMeter());
        commands.insert(QString("extract"), new Extract());
        commands.insert(QString("ls"), new List());
        commands.insert(QString("locate"), new Locate());
        commands.insert(QString("merge"), new Merge());
        commands.insert(QString("rm"), new Remove());
        commands.insert(QString("show"), new Show());
    }
}

Command* Command::getCommand(QString commandName)
{
    populateCommands();
    if (commands.contains(commandName)) {
        return commands[commandName];
    }
    return nullptr;
}

QList<Command*> Command::getCommands()
{
    populateCommands();
    return commands.values();
}
