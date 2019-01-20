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
#include "Diceware.h"
#include "Edit.h"
#include "Estimate.h"
#include "Extract.h"
#include "Generate.h"
#include "List.h"
#include "Locate.h"
#include "Merge.h"
#include "Remove.h"
#include "Show.h"

const QCommandLineOption Command::QuietOption =
    QCommandLineOption(QStringList() << "q"
                                     << "quiet",
                       QObject::tr("Silence password prompt and other secondary outputs."));

const QCommandLineOption Command::KeyFileOption = QCommandLineOption(QStringList() << "k"
                                                                                   << "key-file",
                                                                     QObject::tr("Key file of the database."),
                                                                     QObject::tr("path"));

QMap<QString, Command*> commands;

Command::~Command()
{
}

QString Command::getDescriptionLine()
{

    QString response = name;
    QString space(" ");
    QString spaces = space.repeated(15 - name.length());
    response = response.append(spaces);
    response = response.append(description);
    response = response.append("\n");
    return response;
}

void populateCommands()
{
    if (commands.isEmpty()) {
        commands.insert(QString("add"), new Add());
        commands.insert(QString("clip"), new Clip());
        commands.insert(QString("create"), new Create());
        commands.insert(QString("diceware"), new Diceware());
        commands.insert(QString("edit"), new Edit());
        commands.insert(QString("estimate"), new Estimate());
        commands.insert(QString("extract"), new Extract());
        commands.insert(QString("generate"), new Generate());
        commands.insert(QString("locate"), new Locate());
        commands.insert(QString("ls"), new List());
        commands.insert(QString("merge"), new Merge());
        commands.insert(QString("rm"), new Remove());
        commands.insert(QString("show"), new Show());
    }
}

Command* Command::getCommand(const QString& commandName)
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
