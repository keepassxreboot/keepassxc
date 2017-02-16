/*
 *  Copyright (C) 2017 KeePassXC Team
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

#include "List.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"

void printGroup(Group* group, QString baseName, int depth) {

    QTextStream out(stdout);

    QString groupName = baseName + group->name() + "/";
    QString indentation = QString("  ").repeated(depth);

    out << indentation << groupName << "\n";
    out.flush();

    if (group->entries().isEmpty() && group->children().isEmpty()) {
        out << indentation << "  [empty]\n";
        return;
    }

    for (Entry* entry : group->entries()) {
      out << indentation << "  " << entry->title() << " " << entry->uuid().toHex() << "\n";
    }

    for (Group* innerGroup : group->children()) {
        printGroup(innerGroup, groupName, depth + 1);
    }

}

int List::execute(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main",
                                                                 "List database entries."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "path of the database."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    static QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QString line = inputTextStream.readLine();
    CompositeKey key = CompositeKey::readFromLine(line);

    QString databaseFilename = args.at(0);
    QFile dbFile(databaseFilename);
    if (!dbFile.exists()) {
        qCritical("File %s does not exist.", qPrintable(databaseFilename));
        return EXIT_FAILURE;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open file %s.", qPrintable(databaseFilename));
        return EXIT_FAILURE;
    }

    KeePass2Reader reader;
    Database* db = reader.readDatabase(&dbFile, key);

    if (reader.hasError()) {
        qCritical("Error while parsing the database:\n%s\n", qPrintable(reader.errorString()));
        return EXIT_FAILURE;
    }

    printGroup(db->rootGroup(), QString(""), 0);
    return EXIT_SUCCESS;
}
