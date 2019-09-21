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

#include "DatabaseCommand.h"

#include "Utils.h"

DatabaseCommand::DatabaseCommand()
{
    positionalArguments.append({QString("database"), QObject::tr("Path of the database."), QString("")});
    options.append(Command::KeyFileOption);
    options.append(Command::NoPasswordOption);
#ifdef WITH_XC_YUBIKEY
    options.append(Command::YubiKeyOption);
#endif
}

int DatabaseCommand::execute(const QStringList& arguments)
{
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    const QStringList args = parser->positionalArguments();
    auto db = Utils::unlockDatabase(args.at(0),
                                    !parser->isSet(Command::NoPasswordOption),
                                    parser->value(Command::KeyFileOption),
                                    parser->value(Command::YubiKeyOption),
                                    parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    return executeWithDatabase(db, parser);
}
