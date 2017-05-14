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

#include "Clip.h"

#include <QApplication>
#include <QClipboard>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "gui/Clipboard.h"
#include "keys/CompositeKey.h"

int Clip::execute(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Copy a password to the clipboard"));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("entry", QCoreApplication::translate("main", "Name of the entry to clip."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    out << "Insert the database password\n> ";
    out.flush();

    static QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QString line = inputTextStream.readLine();
    CompositeKey key = CompositeKey::readFromLine(line);

    Database* db = Database::openDatabaseFile(args.at(0), key);
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    QString entryId = args.at(1);
    if (!Uuid::isUuid(entryId)) {
        qCritical("Invalid Uuid %s.", qPrintable(entryId));
        return EXIT_FAILURE;
    }

    Uuid entryUuid = Uuid::fromHex(entryId);
    Entry* entry = db->resolveEntry(entryUuid);
    if (entry == nullptr) {
        qCritical("No entry found with Uuid %s.", qPrintable(entryId));
        return EXIT_FAILURE;
    }

    Clipboard::instance()->setText(entry->password());
    return EXIT_SUCCESS;
}
