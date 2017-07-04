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

#include "Merge.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "gui/UnlockDatabaseDialog.h"

Merge::Merge()
{
    this->name = QString("merge");
    this->description = QString("Merge two databases.");
}

Merge::~Merge()
{
}

int Merge::execute(int argc, char** argv)
{

    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Merge two databases."));
    parser.addPositionalArgument("database1",
                                 QCoreApplication::translate("main", "Path of the database to merge into."));
    parser.addPositionalArgument("database2",
                                 QCoreApplication::translate("main", "Path of the database to merge from."));

    QCommandLineOption samePasswordOption(
        QStringList() << "s"
                      << "same-password",
        QCoreApplication::translate("main", "Use the same password for both database files."));

    QCommandLineOption guiPrompt(QStringList() << "g"
                                               << "gui-prompt",
                                 QCoreApplication::translate("main", "Use a GUI prompt unlocking the database."));
    parser.addOption(guiPrompt);

    parser.addOption(samePasswordOption);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        QCoreApplication app(argc, argv);
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db1;
    Database* db2;

    if (parser.isSet("gui-prompt")) {
        QApplication app(argc, argv);
        db1 = UnlockDatabaseDialog::openDatabasePrompt(args.at(0));
        if (!parser.isSet("same-password")) {
            db2 = UnlockDatabaseDialog::openDatabasePrompt(args.at(1));
        } else {
            db2 = Database::openDatabaseFile(args.at(1), *(db1->key().clone()));
        }
    } else {
        QCoreApplication app(argc, argv);
        db1 = Database::unlockFromStdin(args.at(0));
        if (!parser.isSet("same-password")) {
            db2 = Database::unlockFromStdin(args.at(1));
        } else {
            db2 = Database::openDatabaseFile(args.at(1), *(db1->key().clone()));
        }
    }
    if (db1 == nullptr) {
        return EXIT_FAILURE;
    }
    if (db2 == nullptr) {
        return EXIT_FAILURE;
    }

    db1->merge(db2);
    QString errorMessage = db1->saveToFile(args.at(0));
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    out << "Successfully merged the database files.\n";
    return EXIT_SUCCESS;
}
