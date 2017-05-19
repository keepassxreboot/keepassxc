/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSaveFile>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "format/KeePass2Writer.h"
#include "keys/CompositeKey.h"

int Merge::execute(int argc, char** argv)
{

    QCoreApplication app(argc, argv);
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

    parser.addOption(samePasswordOption);
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    out << "Insert the first database password\n> ";
    out.flush();

    static QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QString line1 = inputTextStream.readLine();
    CompositeKey key1 = CompositeKey::readFromLine(line1);

    CompositeKey key2;
    if (parser.isSet("same-password")) {
        key2 = *key1.clone();
    } else {
        out << "Insert the second database password\n> ";
        out.flush();
        QString line2 = inputTextStream.readLine();
        key2 = CompositeKey::readFromLine(line2);
    }

    Database* db1 = Database::openDatabaseFile(args.at(0), key1);
    if (db1 == nullptr) {
        return EXIT_FAILURE;
    }

    Database* db2 = Database::openDatabaseFile(args.at(1), key2);
    if (db2 == nullptr) {
        return EXIT_FAILURE;
    }

    db1->merge(db2);

    QSaveFile saveFile(args.at(0));
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qCritical("Unable to open file %s for writing.", qPrintable(args.at(0)));
        return EXIT_FAILURE;
    }

    KeePass2Writer writer;
    writer.writeDatabase(&saveFile, db1);

    if (writer.hasError()) {
        qCritical("Error while updating the database:\n%s\n", qPrintable(writer.errorString()));
        return EXIT_FAILURE;
    }

    if (!saveFile.commit()) {
        qCritical("Error while updating the database:\n%s\n", qPrintable(writer.errorString()));
        return EXIT_FAILURE;
    }

    out << "Successfully merged the database files.\n";
    return EXIT_SUCCESS;
}
