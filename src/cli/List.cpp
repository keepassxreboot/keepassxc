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
#include <stdio.h>

#include "List.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "gui/UnlockDatabaseDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "keys/CompositeKey.h"


int List::execute(int argc, char** argv)
{
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "List database entries."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("group",
                                 QCoreApplication::translate("main", "Path of the group to list. Default is /"),
                                 QString("[group]"));
    QCommandLineOption printUuidsOption(
        QStringList() << "u"
                      << "print-uuids",
        QCoreApplication::translate("main", "Print the UUIDs of the entries and groups."));
    parser.addOption(printUuidsOption);
    QCommandLineOption guiPrompt(
        QStringList() << "g"
                      << "gui-prompt",
        QCoreApplication::translate("main", "Use a GUI prompt unlocking the database."));
    parser.addOption(guiPrompt);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1 && args.size() != 2) {
        QCoreApplication app(argc, argv);
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = nullptr;
    if (parser.isSet("gui-prompt")) {
        QApplication app(argc, argv);
        db = UnlockDatabaseDialog::openDatabasePrompt(args.at(0));
    } else {
        QCoreApplication app(argc, argv);
        db = Database::unlockFromStdin(args.at(0));
    }

    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    Group* group = db->rootGroup();
    if (args.size() == 2) {
        QString groupPath = args.at(1);
        group = db->rootGroup()->findGroupByPath(groupPath);
        if (group == nullptr) {
            qCritical("Cannot find group %s.", qPrintable(groupPath));
            return EXIT_FAILURE;
        }
    }

    out << group->print(parser.isSet("print-uuids"));
    out.flush();
    return EXIT_SUCCESS;
}
