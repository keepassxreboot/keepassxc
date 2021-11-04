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

#include "AttachmentExport.h"

#include "Utils.h"
#include "core/Group.h"

#include <QCommandLineParser>
#include <QFile>

const QCommandLineOption AttachmentExport::StdoutOption =
    QCommandLineOption(QStringList() << "stdout", QObject::tr(""));

AttachmentExport::AttachmentExport()
{
    name = QString("attachment-export");
    description = QObject::tr("Export an attachment of an entry.");
    options.append(AttachmentExport::StdoutOption);
    positionalArguments.append(
        {QString("entry"), QObject::tr("Path of the entry with the target attachment."), QString("")});
    positionalArguments.append(
        {QString("attachment-name"), QObject::tr("Name of the attachment to be exported."), QString("")});
    optionalArguments.append(
        {QString("export-file"), QObject::tr("Path to which the attachment should be exported."), QString("")});
}

int AttachmentExport::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    auto args = parser->positionalArguments();
    auto entryPath = args.at(1);

    auto entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    auto attachmentName = args.at(2);

    auto attachments = entry->attachments();
    if (!attachments->hasKey(attachmentName)) {
        err << QObject::tr("Could not find attachment with name %1.").arg(attachmentName) << endl;
        return EXIT_FAILURE;
    }

    if (parser->isSet(AttachmentExport::StdoutOption)) {
        // Output to STDOUT even in quiet mode
        Utils::STDOUT << attachments->value(attachmentName) << flush;
        return EXIT_SUCCESS;
    }

    if (args.size() < 4) {
        err << QObject::tr("No export target given. Please use '--stdout' or specify an 'export-file'.") << endl;
        return EXIT_FAILURE;
    }

    auto exportFileName = args.at(3);
    QFile exportFile(exportFileName);
    if (!exportFile.open(QIODevice::WriteOnly)) {
        err << QObject::tr("Could not open output file %1.").arg(exportFileName) << endl;
        return EXIT_FAILURE;
    }
    exportFile.write(attachments->value(attachmentName));

    out << QObject::tr("Successfully exported attachment %1 of entry %2 to %3.")
               .arg(attachmentName, entryPath, exportFileName)
        << endl;

    return EXIT_SUCCESS;
}
