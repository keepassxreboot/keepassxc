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

#include "Clip.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>

#include "gui/UnlockDatabaseDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "gui/Clipboard.h"

Clip::Clip()
{
    this->name = QString("clip");
    this->description = QObject::tr("Copy an entry's password to the clipboard.");
}

Clip::~Clip()
{
}

int Clip::execute(int argc, char** argv)
{
    QStringList arguments;
    // Skipping the first argument (keepassxc).
    for (int i = 1; i < argc; ++i) {
        arguments << QString(argv[i]);
    }

    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    QCommandLineOption guiPrompt(QStringList() << "g"
                                               << "gui-prompt",
                                 QObject::tr("Use a GUI prompt unlocking the database."));
    parser.addOption(guiPrompt);
    parser.addPositionalArgument("entry", QObject::tr("Path of the entry to clip."));
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        QCoreApplication app(argc, argv);
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli clip");
        return EXIT_FAILURE;
    }

    Database* db = nullptr;
    QApplication app(argc, argv);
    if (parser.isSet("gui-prompt")) {
        db = UnlockDatabaseDialog::openDatabasePrompt(args.at(0));
    } else {
        db = Database::unlockFromStdin(args.at(0));
    }

    if (!db) {
        return EXIT_FAILURE;
    }
    return this->clipEntry(db, args.at(1));
}

int Clip::clipEntry(Database* database, QString entryPath)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    Entry* entry = database->rootGroup()->findEntry(entryPath);
    if (!entry) {
        qCritical("Entry %s not found.", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    Clipboard::instance()->setText(entry->password());
    return EXIT_SUCCESS;

}
