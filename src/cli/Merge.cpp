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
#include "core/Database.h"
#include "core/Merger.h"
#include "cli/Utils.h"

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

    QCommandLineOption samePasswordOption(QStringList() << "s" << "same-credentials",
                                          QObject::tr("Use the same credentials for both database files."));

    QCommandLineOption keyFile(QStringList() << "k" << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    QCommandLineOption keyFileFrom(QStringList() << "f" << "key-file-from",
                                   QObject::tr("Key file of the database to merge from."),
                                   QObject::tr("path"));
    parser.addOption(keyFileFrom);

    parser.addOption(samePasswordOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli merge");
        return EXIT_FAILURE;
    }

    QScopedPointer<Database> db1(Database::unlockFromStdin(args.at(0), parser.value(keyFile), Utils::STDOUT, Utils::STDERR));
    if (!db1) {
        return EXIT_FAILURE;
    }

    QScopedPointer<Database> db2;
    if (!parser.isSet("same-credentials")) {
        db2.reset(Database::unlockFromStdin(args.at(1), parser.value(keyFileFrom), Utils::STDOUT, Utils::STDERR));
    } else {
        db2.reset(Database::openDatabaseFile(args.at(1), db1->key()));
    }
    if (!db2) {
        return EXIT_FAILURE;
    }

    Merger merger(db2.data(), db1.data());
    merger.merge();

    QString errorMessage = db1->saveToFile(args.at(0));
    if (!errorMessage.isEmpty()) {
        err << QObject::tr("Unable to save database to file : %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    out << "Successfully merged the database files." << endl;
    return EXIT_SUCCESS;
}
