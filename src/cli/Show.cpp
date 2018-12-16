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

#include "Show.h"

#include <cstdlib>
#include <stdio.h>

#include <QCommandLineParser>

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
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
    TextStream out(Utils::STDOUT);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);
    QCommandLineOption totp(QStringList() << "t"
                                          << "totp",
                            QObject::tr("Show the entry's current TOTP."));
    parser.addOption(totp);
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
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli show");
        return EXIT_FAILURE;
    }

    auto db = Utils::unlockDatabase(args.at(0),
                                    parser.value(Command::KeyFileOption),
                                    parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    return showEntry(db.data(), parser.values(attributes), parser.isSet(totp), args.at(1));
}

int Show::showEntry(Database* database, QStringList attributes, bool showTotp, const QString& entryPath)
{
    TextStream in(Utils::STDIN, QIODevice::ReadOnly);
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    if (showTotp && !entry->hasTotp()) {
        err << QObject::tr("Entry with path %1 has no TOTP set up.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    // If no attributes specified, output the default attribute set.
    bool showAttributeNames = attributes.isEmpty() && !showTotp;
    if (attributes.isEmpty() && !showTotp) {
        attributes = EntryAttributes::DefaultAttributes;
    }

    // Iterate over the attributes and output them line-by-line.
    bool sawUnknownAttribute = false;
    for (const QString& attribute : asConst(attributes)) {
        if (!entry->attributes()->contains(attribute)) {
            sawUnknownAttribute = true;
            err << QObject::tr("ERROR: unknown attribute %1.").arg(attribute) << endl;
            continue;
        }
        if (showAttributeNames) {
            out << attribute << ": ";
        }
        out << entry->resolveMultiplePlaceholders(entry->attributes()->value(attribute)) << endl;
    }

    if (showTotp) {
        if (showAttributeNames) {
            out << "TOTP: ";
        }
        out << entry->totp() << endl;
    }

    return sawUnknownAttribute ? EXIT_FAILURE : EXIT_SUCCESS;
}
