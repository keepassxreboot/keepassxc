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

#include "Merge.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Merger.h"

const QCommandLineOption SameCredentialsOption =
    QCommandLineOption(QStringList() << "s"
                                     << "same-credentials",
                       QObject::tr("Use the same credentials for both database files."));

const QCommandLineOption KeyFileFromOption =
    QCommandLineOption(QStringList() << "key-file-from",
                       QObject::tr("Key file of the database to merge from."),
                       QObject::tr("path"));

const QCommandLineOption NoPasswordFromOption =
    QCommandLineOption(QStringList() << "no-password-from",
                       QObject::tr("Deactivate password key for the database to merge from."));

const QCommandLineOption DryRunOption =
    QCommandLineOption(QStringList() << "dry-run",
                       QObject::tr("Only print the changes detected by the merge operation."));

#ifdef WITH_XC_YUBIKEY
const QCommandLineOption YubiKeyFromOption(QStringList() << "yubikey-from",
                                           QObject::tr("Yubikey slot for the second database."),
                                           QObject::tr("slot"));
#endif


CommandArgs Merge::getParserArgs(const CommandCtx& ctx) const
{
    static const CommandArgs args {
        { {"database2", QObject::tr("Path of the database to merge from."), ""} },
        {},
        {
            SameCredentialsOption,
            KeyFileFromOption,
            NoPasswordFromOption,
            DryRunOption,
#ifdef WITH_XC_YUBIKEY
            YubiKeyFromOption
#endif
        }
    };
    return DatabaseCommand::getParserArgs(ctx).merge(args);
}

int Merge::executeWithDatabase(CommandCtx& ctx, const QCommandLineParser& parser)
{
    auto& out = parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser.positionalArguments();
    Database& database = ctx.getDb();
    const int db2ArgIdx = ctx.getRunmode() == Runmode::InteractiveCmd ? 0 : 1;
    Q_ASSERT(args.size() == db2ArgIdx + 1);
    const QString& toDatabasePath = ctx.getDb().filePath();
    const QString& fromDatabasePath = args.at(db2ArgIdx);

    std::unique_ptr<Database> db2;
    if (!parser.isSet(SameCredentialsOption)) {
        db2 = openDatabase(parser);
    } else {
        db2 = Utils::make_unique<Database>();
        QString errorMessage;
        if (!db2->open(fromDatabasePath, database.key(), &errorMessage, false)) {
            err << QObject::tr("Error reading merge file:\n%1").arg(errorMessage);
            return EXIT_FAILURE;
        }
    }

    Merger merger(db2.get(), &database);
    QStringList changeList = merger.merge();

    for (auto& mergeChange : changeList) {
        out << "\t" << mergeChange << endl;
    }

    if (!changeList.isEmpty() && !parser.isSet(DryRunOption)) {
        QString errorMessage;
        if (!database.save(&errorMessage, true, false)) {
            err << QObject::tr("Unable to save database to file : %1").arg(errorMessage) << endl;
            return EXIT_FAILURE;
        }
        out << QObject::tr("Successfully merged %1 into %2.").arg(fromDatabasePath, toDatabasePath) << endl;
    } else {
        out << QObject::tr("Database was not modified by merge operation.") << endl;
    }

    return EXIT_SUCCESS;
}
