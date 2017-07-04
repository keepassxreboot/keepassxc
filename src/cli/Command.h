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

#ifndef KEEPASSXC_COMMAND_H
#define KEEPASSXC_COMMAND_H

#include <QList>
#include <QString>
#include <QStringList>

#include "core/Database.h"

class Command
{
public:
    virtual ~Command();
    virtual int execute(int argc, char** argv);
    QString name;
    QString description;
    QString getDescriptionLine();

    static QList<Command*> getCommands();
    static Command* getCommand(QString commandName);
};

#endif // KEEPASSXC_COMMAND_H
