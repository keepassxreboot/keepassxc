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

#include "Utils.h"
#include "core/Group.h"
#include "core/Tools.h"

#include <QCommandLineParser>

const QCommandLineOption Show::TotpOption =
    QCommandLineOption(QStringList() << "t" << "totp", QObject::tr("Only show the entry's current TOTP."));

const QCommandLineOption Show::UuidOption =
    QCommandLineOption(QStringList() << "uuid", QObject::tr("Show the entry's UUID."));

const QCommandLineOption Show::TagsOption =
    QCommandLineOption(QStringList() << "tags", QObject::tr("Show the entry's tags."));

const QCommandLineOption Show::ProtectedAttributesOption =
    QCommandLineOption(QStringList() << "s" << "show-protected",
                       QObject::tr("Show the protected attributes in clear text."));

const QCommandLineOption Show::AllAttributesOption =
    QCommandLineOption(QStringList() << "all",
                       QObject::tr("Show all the attributes of the entry, including UUID and Tags."));

const QCommandLineOption Show::AttachmentsOption =
    QCommandLineOption(QStringList() << "show-attachments", QObject::tr("Show the attachments of the entry."));

const QCommandLineOption Show::AttributesOption = QCommandLineOption(
    QStringList() << "a" << "attributes",
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
    options.append(Show::UuidOption);
    options.append(Show::TagsOption);
    options.append(Show::AttributesOption);
    options.append(Show::ProtectedAttributesOption);
    options.append(Show::AllAttributesOption);
    options.append(Show::AttachmentsOption);
    positionalArguments.append({QString("entry"), QObject::tr("Name of the entry to show."), QString("")});
}

int Show::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    const QString& entryPath = args.at(1);
    bool showProtectedAttributes = parser->isSet(Show::ProtectedAttributesOption);
    bool showAllAttributes = parser->isSet(Show::AllAttributesOption);
    bool showUuid = parser->isSet(Show::UuidOption);
    bool showTags = parser->isSet(Show::TagsOption);
    QStringList attributes = parser->values(Show::AttributesOption);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not find entry with path %1.").arg(entryPath) << Qt::endl;
        return EXIT_FAILURE;
    }

    // Early exit if the user only wants to show the TOTP
    if (parser->isSet(Show::TotpOption)) {
        if (!entry->hasTotp()) {
            err << QObject::tr("Entry with path %1 has no TOTP set up.").arg(entryPath) << Qt::endl;
            return EXIT_FAILURE;
        }

        out << entry->totp() << Qt::endl;
        return EXIT_SUCCESS;
    }

    bool attributesWereSpecified = !showUuid && !showTags;
    if (showAllAttributes) {
        attributesWereSpecified = false;
        showUuid = true;
        showTags = true;
        attributes = EntryAttributes::DefaultAttributes;
        // Adding the custom attributes after the default attributes so that
        // the default attributes are always shown first.
        for (QString attributeName : entry->attributes()->keys()) {
            if (EntryAttributes::DefaultAttributes.contains(attributeName)) {
                continue;
            }
            attributes.append(attributeName);
        }
    } else if (attributes.isEmpty() && !showUuid && !showTags) {
        // If no attributes are specified, output the default attribute set.
        attributesWereSpecified = false;
        attributes = EntryAttributes::DefaultAttributes;
        showTags = true;
    }

    // Iterate over the attributes and output them line-by-line.
    bool encounteredError = false;
    for (const QString& attributeName : asConst(attributes)) {
        QStringList attrs = Utils::findAttributes(*entry->attributes(), attributeName);
        if (attrs.isEmpty()) {
            encounteredError = true;
            err << QObject::tr("ERROR: unknown attribute %1.").arg(attributeName) << Qt::endl;
            continue;
        } else if (attrs.size() > 1) {
            encounteredError = true;
            err << QObject::tr("ERROR: attribute %1 is ambiguous, it matches %2.")
                       .arg(attributeName, QLocale().createSeparatedList(attrs))
                << Qt::endl;
            continue;
        }
        QString canonicalName = attrs[0];
        if (!attributesWereSpecified) {
            out << canonicalName << ": ";
        }
        if (entry->attributes()->isProtected(canonicalName) && !attributesWereSpecified && !showProtectedAttributes) {
            out << "PROTECTED" << Qt::endl;
        } else {
            out << entry->resolveMultiplePlaceholders(entry->attributes()->value(canonicalName)) << Qt::endl;
        }
    }

    // Output UUID and Tags if a certain field wasn't specified
    if (showTags) {
        out << "Tags: " << entry->tags() << Qt::endl;
    }
    if (showUuid) {
        out << "UUID: " << entry->uuid().toString() << Qt::endl;
    }

    if (parser->isSet(Show::AttachmentsOption)) {
        // Separate attachment output from attributes output via a newline.
        out << Qt::endl;

        EntryAttachments* attachments = entry->attachments();
        if (attachments->isEmpty()) {
            out << QObject::tr("No attachments present.") << Qt::endl;
        } else {
            out << QObject::tr("Attachments:") << Qt::endl;

            // Iterate over the attachments and output their names and size line-by-line, indented.
            for (const QString& attachmentName : attachments->keys()) {
                // TODO: use QLocale::formattedDataSize when >= Qt 5.10
                QString attachmentSize = Tools::humanReadableFileSize(attachments->value(attachmentName).size(), 1);
                out << "  " << attachmentName << " (" << attachmentSize << ")" << Qt::endl;
            }
        }
    }

    return encounteredError ? EXIT_FAILURE : EXIT_SUCCESS;
}
