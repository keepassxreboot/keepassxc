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

#include "Show.h"

#include <QCommandLineParser>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

Show::Show()
{
    name = QString("show");
    description = QObject::tr("Show an entry's information.");
}

Show::~Show()
{
}

int Show::execute(const QStringList& arguments)
{
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    QCommandLineOption keyFile(QStringList() << "k"
                                             << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    QCommandLineOption attributes(
        QStringList() << "a"
                      << "attributes",
        QObject::tr(
            "Names of the attributes to show. "
            "This option can be specified more than once, with each attribute shown one-per-line in the given order. "
            "If no attributes are specified, a summary of the default attributes is given."),
        QObject::tr("attribute"));
    parser.addOption(attributes);
    parser.addPositionalArgument("entry", QObject::tr("Name of the entry to show."));
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli show");
        return EXIT_FAILURE;
    }

    Database* db = Database::unlockFromStdin(args.at(0), parser.value(keyFile));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->showEntries(db, parser.values(attributes), args.at(1));
}

int Show::showEntries(Database* database, QStringList attributes, QString entryPath)
{
    int result = EXIT_SUCCESS;
    QMap<QUuid, Entry*> entries = database->rootGroup()->findEntries(entryPath);

    if(entries.isEmpty()) {
        qCritical("could not find entry with path %s.", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    // If no attributes specified, output the default attribute set.
    bool showAttributeNames = attributes.isEmpty();
    if (attributes.isEmpty()) {
        attributes = EntryAttributes::DefaultAttributes;
    }

    for(Entry* entry : entries.values()) {
        if(showEntry(entry, attributes, showAttributeNames) != EXIT_SUCCESS) {
            result = EXIT_FAILURE;
        }
    }

    return result;
}

int Show::showEntry(Entry* entry, QStringList attributes, bool showAttributeNames)
{
    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    // Iterate over the attributes and output them line-by-line.
    bool sawUnknownAttribute = false;
    for (QString attribute : attributes) {
        if (!entry->attributes()->contains(attribute)) {
            sawUnknownAttribute = true;
            qCritical("ERROR: unknown attribute '%s'.", qPrintable(attribute));
            continue;
        }
        if (showAttributeNames) {
            outputTextStream << attribute << ": ";
        }
        outputTextStream << entry->resolveMultiplePlaceholders(entry->attributes()->value(attribute)) << endl;
    }
    outputTextStream << "---" << endl;
    return sawUnknownAttribute ? EXIT_FAILURE : EXIT_SUCCESS;
}
