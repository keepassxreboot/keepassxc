/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"

#include <QLocale>

const QCommandLineOption Show::TotpOption = QCommandLineOption(QStringList() << "t"
                                                                             << "totp",
                                                               QObject::tr("Show the entry's current TOTP."));

const QCommandLineOption Show::ProtectedAttributesOption =
    QCommandLineOption(QStringList() << "s"
                                     << "show-protected",
                       QObject::tr("Show the protected attributes in clear text."));

const QCommandLineOption Show::AttributesOption = QCommandLineOption(
    QStringList() << "a"
                  << "attributes",
    QObject::tr(
        "Names of the attributes to show. "
        "This option can be specified more than once, with each attribute shown one-per-line in the given order. "
        "If no attributes are specified, a summary of the default attributes is given."),
    QObject::tr("attribute"));

Show::Show()
{
    name = QString("show");
    description = QObject::tr("Show an entry's information.");
    options.append(Show::TotpOption);
    options.append(Show::AttributesOption);
    options.append(Show::ProtectedAttributesOption);
    positionalArguments.append({QString("entry"), QObject::tr("Name of the entry to show."), QString("")});
}

int Show::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    const QString& entryPath = args.at(1);
    bool showTotp = parser->isSet(Show::TotpOption);
    bool showProtectedAttributes = parser->isSet(Show::ProtectedAttributesOption);
    QStringList attributes = parser->values(Show::AttributesOption);

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
    bool showDefaultAttributes = attributes.isEmpty() && !showTotp;
    if (showDefaultAttributes) {
        attributes = EntryAttributes::DefaultAttributes;
    }

    // Iterate over the attributes and output them line-by-line.
    bool encounteredError = false;
    for (const QString& attributeName : asConst(attributes)) {
        QStringList attrs = Utils::findAttributes(*entry->attributes(), attributeName);
        if (attrs.isEmpty()) {
            encounteredError = true;
            err << QObject::tr("ERROR: unknown attribute %1.").arg(attributeName) << endl;
            continue;
        } else if (attrs.size() > 1) {
            encounteredError = true;
            err << QObject::tr("ERROR: attribute %1 is ambiguous, it matches %2.")
                       .arg(attributeName, QLocale().createSeparatedList(attrs))
                << endl;
            continue;
        }
        QString canonicalName = attrs[0];
        if (showDefaultAttributes) {
            out << canonicalName << ": ";
        }
        if (entry->attributes()->isProtected(canonicalName) && showDefaultAttributes && !showProtectedAttributes) {
            out << "PROTECTED" << endl;
        } else {
            out << entry->resolveMultiplePlaceholders(entry->attributes()->value(canonicalName)) << endl;
        }
    }

    if (showTotp) {
        out << entry->totp() << endl;
    }

    return encounteredError ? EXIT_FAILURE : EXIT_SUCCESS;
}
