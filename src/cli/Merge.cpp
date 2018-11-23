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

#include "Merge.h"

#include <QCommandLineParser>

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Merger.h"

#include <cstdlib>

Merge::Merge()
{
    name = QString("merge");
    description = QObject::tr("Merge two databases.");
}

Merge::~Merge()
{
}

int Merge::execute(const QStringList& arguments)
{
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database1", QObject::tr("Path of the database to merge into."));
    parser.addPositionalArgument("database2", QObject::tr("Path of the database to merge from."));
    parser.addOption(Command::QuietOption);

    QCommandLineOption samePasswordOption(QStringList() << "s" << "same-credentials",
                                          QObject::tr("Use the same credentials for both database files."));
    parser.addOption(samePasswordOption);
    parser.addOption(Command::KeyFileOption);

    QCommandLineOption keyFileFromOption(QStringList() << "f" << "key-file-from",
                                         QObject::tr("Key file of the database to merge from."),
                                         QObject::tr("path"));
    parser.addOption(keyFileFromOption);

    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli merge");
        return EXIT_FAILURE;
    }

    auto db1 = Database::unlockFromStdin(args.at(0),
                                         parser.value(Command::KeyFileOption),
                                         parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                         Utils::STDERR);
    if (!db1) {
        return EXIT_FAILURE;
    }

    QSharedPointer<Database> db2;
    if (!parser.isSet("same-credentials")) {
        db2 = Database::unlockFromStdin(args.at(1), parser.value(keyFileFromOption), Utils::STDOUT, Utils::STDERR);
    } else {
        db2 = QSharedPointer<Database>::create();
        QString errorMessage;
        if (!db2->open(args.at(1), db1->key(), &errorMessage, false)) {
            err << QObject::tr("Error reading merge file:\n%1").arg(errorMessage);
            return EXIT_FAILURE;
        }
    }

    Merger merger(db2.data(), db1.data());
    bool databaseChanged = merger.merge();

    if (databaseChanged) {
        QString errorMessage;
        if (!db1->save(args.at(0), &errorMessage, true, false)) {
            err << QObject::tr("Unable to save database to file : %1").arg(errorMessage) << endl;
            return EXIT_FAILURE;
        }
        if (!parser.isSet(Command::QuietOption)) {
            out << "Successfully merged the database files." << endl;
        }
    } else if (!parser.isSet(Command::QuietOption)) {
        out << "Database was not modified by merge operation." << endl;
    }

    return EXIT_SUCCESS;
}
