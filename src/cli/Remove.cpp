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

#include "Remove.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"

Remove::Remove()
{
    name = QString("rm");
    description = QString("Remove an entry from the database.");
}

Remove::~Remove()
{
}

int Remove::execute(const QStringList& arguments)
{
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Remove an entry from the database."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    QCommandLineOption keyFile(QStringList() << "k"
                                             << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.addPositionalArgument("entry", QCoreApplication::translate("main", "Path of the entry to remove."));
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        outputTextStream << parser.helpText().replace("keepassxc-cli", "keepassxc-cli rm");
        return EXIT_FAILURE;
    }

    Database* db = Database::unlockFromStdin(args.at(0), parser.value(keyFile));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->removeEntry(db, args.at(0), args.at(1));
}

int Remove::removeEntry(Database* database, QString databasePath, QString entryPath)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        qCritical("Entry %s not found.", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    QString entryTitle = entry->title();
    bool recycled = true;
    if (Tools::hasChild(database->metadata()->recycleBin(), entry) || !database->metadata()->recycleBinEnabled()) {
        delete entry;
        recycled = false;
    } else {
        database->recycleEntry(entry);
    };

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    if (recycled) {
        outputTextStream << "Successfully recycled entry " << entryTitle << "." << endl;
    } else {
        outputTextStream << "Successfully deleted entry " << entryTitle << "." << endl;
    }

    return EXIT_SUCCESS;
}
