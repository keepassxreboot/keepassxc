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

#include "cli/TextStream.h"
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
    TextStream out(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::tr("main", "Remove an entry from the database."));
    parser.addPositionalArgument("database", QCoreApplication::tr("main", "Path of the database."));
    QCommandLineOption keyFile(QStringList() << "k" << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.addPositionalArgument("entry", QCoreApplication::tr("main", "Path of the entry to remove."));
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli rm");
        return EXIT_FAILURE;
    }

    QScopedPointer<Database> db(Database::unlockFromStdin(args.at(0), parser.value(keyFile), Utils::STDOUT, Utils::STDERR));
    if (!db) {
        return EXIT_FAILURE;
    }

    return removeEntry(db.data(), args.at(0), args.at(1));
}

int Remove::removeEntry(Database* database, const QString& databasePath, const QString& entryPath)
{
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    QPointer<Entry> entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Entry %1 not found.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    QString entryTitle = entry->title();
    bool recycled = true;
    auto* recycleBin = database->metadata()->recycleBin();
    if (!database->metadata()->recycleBinEnabled() || (recycleBin && recycleBin->findEntryByUuid(entry->uuid()))) {
        delete entry;
        recycled = false;
    } else {
        database->recycleEntry(entry);
    };

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        err << QObject::tr("Unable to save database to file: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (recycled) {
        out << QObject::tr("Successfully recycled entry %1.").arg(entryTitle) << endl;
    } else {
        out << QObject::tr("Successfully deleted entry %1.").arg(entryTitle) << endl;
    }

    return EXIT_SUCCESS;
}
