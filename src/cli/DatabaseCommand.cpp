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


const QCommandLineOption KeyFileOption = QCommandLineOption(QStringList() << "k" << "key-file",
                                                                             QObject::tr("Key file of the database."),
                                                                             QObject::tr("path"));

const QCommandLineOption NoPasswordOption =
    QCommandLineOption(QStringList() << "no-password", QObject::tr("Deactivate password key for the database."));

#ifdef WITH_XC_YUBIKEY
const QCommandLineOption YubiKeyOption =
    QCommandLineOption(QStringList() << "y"
                                     << "yubikey",
                       QObject::tr("Yubikey slot and optional serial used to access the database (e.g., 1:7370001)."),
                       QObject::tr("slot[:serial]"));
#endif


CommandArgs DatabaseCommand::getParserArgs(const CommandCtx& ctx) const
{
    if (ctx.getRunmode() == Runmode::InteractiveCmd)
        return {};

    static const CommandArgs dbArgs = {
        { {QString("database"), QObject::tr("Path of the database."), QString("")} },
        {},
        {
            KeyFileOption,
#ifdef WITH_XC_YUBIKEY
            YubiKeyOption,
#endif
            NoPasswordOption }
    };
    return dbArgs;
}

std::unique_ptr<Database> DatabaseCommand::openDatabase(const QCommandLineParser& parser)
{
    return Utils::unlockDatabase(parser.positionalArguments().first(),
                                 !parser.isSet(NoPasswordOption),
                                 parser.value(KeyFileOption),
#ifdef WITH_XC_YUBIKEY
                                 parser.value(YubiKeyOption),
#else
                                 "",
#endif
                                 parser.isSet(Command::QuietOption));
}

int DatabaseCommand::execImpl(CommandCtx& ctx, const QCommandLineParser& parser)
{
    if (ctx.hasDb()) {
        return executeWithDatabase(ctx, parser);
    }
    std::unique_ptr<Database> db = openDatabase(parser);
    BREAK_IF(!db, EXIT_FAILURE,
             ctx, QString("Error opening database '%1'.").arg(parser.positionalArguments().first()));
    ctx.setDb(std::move(db));
    return executeWithDatabase(ctx, parser);
}
