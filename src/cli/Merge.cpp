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

const QCommandLineOption Merge::SameCredentialsOption =
    QCommandLineOption(QStringList() << "s"
                                     << "same-credentials",
                       QObject::tr("Use the same credentials for both database files."));

const QCommandLineOption Merge::KeyFileFromOption =
    QCommandLineOption(QStringList() << "k"
                                     << "key-file-from",
                       QObject::tr("Key file of the database to merge from."),
                       QObject::tr("path"));

const QCommandLineOption Merge::NoPasswordFromOption =
    QCommandLineOption(QStringList() << "no-password-from",
                       QObject::tr("Deactivate password key for the database to merge from."));

Merge::Merge()
{
    name = QString("merge");
    description = QObject::tr("Merge two databases.");
    options.append(Merge::SameCredentialsOption);
    options.append(Merge::KeyFileFromOption);
    options.append(Merge::NoPasswordFromOption);
    positionalArguments.append({QString("database2"), QObject::tr("Path of the database to merge from."), QString("")});
}

Merge::~Merge()
{
}

int Merge::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    const QStringList args = parser->positionalArguments();

    QSharedPointer<Database> db2;
    if (!parser->isSet(Merge::SameCredentialsOption)) {
        db2 = Utils::unlockDatabase(args.at(1),
                                    !parser->isSet(Merge::NoPasswordFromOption),
                                    parser->value(Merge::KeyFileFromOption),
                                    parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
        if (!db2) {
            return EXIT_FAILURE;
        }
    } else {
        db2 = QSharedPointer<Database>::create();
        QString errorMessage;
        if (!db2->open(args.at(1), database->key(), &errorMessage, false)) {
            errorTextStream << QObject::tr("Error reading merge file:\n%1").arg(errorMessage);
            return EXIT_FAILURE;
        }
    }

    Merger merger(db2.data(), database.data());
    bool databaseChanged = merger.merge();

    if (databaseChanged) {
        QString errorMessage;
        if (!database->save(args.at(0), &errorMessage, true, false)) {
            errorTextStream << QObject::tr("Unable to save database to file : %1").arg(errorMessage) << endl;
            return EXIT_FAILURE;
        }
        if (!parser->isSet(Command::QuietOption)) {
            outputTextStream << "Successfully merged the database files." << endl;
        }
    } else if (!parser->isSet(Command::QuietOption)) {
        outputTextStream << "Database was not modified by merge operation." << endl;
    }

    return EXIT_SUCCESS;
}
