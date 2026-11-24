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

#include "Export.h"

#include "TextStream.h"
#include "Utils.h"
#include "core/Global.h"
#include "format/CsvExporter.h"
#include "format/HtmlExporter.h"

#include <QCommandLineParser>
#include <QFileDevice>

const QCommandLineOption Export::FormatOption = QCommandLineOption(
    QStringList() << "f" << "format",
    QObject::tr("Format to use when exporting. Available choices are 'xml', 'csv' or 'html'. Defaults to 'xml'."),
    QStringLiteral("xml|csv|html"));

Export::Export()
{
    name = QStringLiteral("export");
    options.append(Export::FormatOption);
    description = QObject::tr("Exports the content of a database to standard output in the specified format.");
}

int Export::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream out(Utils::STDOUT.device());
    auto& err = Utils::STDERR;

    QString format = parser->value(Export::FormatOption);
    if (format.isEmpty() || format.startsWith(QStringLiteral("xml"), Qt::CaseInsensitive)) {
        // write the XML straight to stdout instead of building all of it in memory first. If writing fails
        // part way, what was already written stays on stdout; the exit code and the message on stderr report it.
        QString errorMessage;
        QStringList hiddenCharacterPlaces;
        out.flush();
        const bool exported = database->extract(out.device(), &errorMessage, &hiddenCharacterPlaces);
        // stdout is a buffered file; QTextStream used to flush it, writing to the device directly does not
        if (auto* file = qobject_cast<QFileDevice*>(out.device())) {
            file->flush();
        }
        if (!exported) {
            err << QObject::tr("Unable to export database to XML: %1").arg(errorMessage) << Qt::endl;
            return EXIT_FAILURE;
        }
        // say which values XML could not hold as text. They are stored as base64 and nothing is lost; this
        // only tells the person what is in the file. (stderr, so it never mixes into the XML on stdout.)
        if (!hiddenCharacterPlaces.isEmpty() && !parser->isSet(Command::QuietOption)) {
            err << QObject::tr("%1 value(s) contain characters XML cannot store as text (for example hidden control "
                               "characters). They were saved as base64 and are restored exactly when this file is "
                               "imported into KeePassXC:")
                       .arg(hiddenCharacterPlaces.size())
                << Qt::endl;
            for (const auto& place : hiddenCharacterPlaces) {
                err << "  " << place << Qt::endl;
            }
        }
    } else if (format.startsWith(QStringLiteral("csv"), Qt::CaseInsensitive)) {
        CsvExporter csvExporter;
        out << csvExporter.exportDatabase(database);
    } else if (format.startsWith(QStringLiteral("html"), Qt::CaseInsensitive)) {
        HtmlExporter htmlExporter;
        out << htmlExporter.exportDatabase(database);
    } else {
        err << QObject::tr("Unsupported format %1").arg(format) << Qt::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
