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

#include "Analyze.h"
#include "cli/Utils.h"
#include "core/HibpOffline.h"

#include <QCommandLineParser>
#include <QFile>
#include <QString>

#include "cli/TextStream.h"
#include "core/Group.h"
#include "core/Tools.h"

const QCommandLineOption Analyze::HIBPDatabaseOption = QCommandLineOption(
    {"H", "hibp"},
    QObject::tr("Check if any passwords have been publicly leaked. FILENAME must be the path of a file listing "
                "SHA-1 hashes of leaked passwords in HIBP format, as available from "
                "https://haveibeenpwned.com/Passwords."),
    QObject::tr("FILENAME"));

Analyze::Analyze()
{
    name = QString("analyze");
    description = QObject::tr("Analyze passwords for weaknesses and problems.");
    options.append(Analyze::HIBPDatabaseOption);
}

int Analyze::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream inputTextStream(Utils::STDIN, QIODevice::ReadOnly);
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QString hibpDatabase = parser->value(Analyze::HIBPDatabaseOption);
    QFile hibpFile(hibpDatabase);
    if (!hibpFile.open(QFile::ReadOnly)) {
        errorTextStream << QObject::tr("Failed to open HIBP file %1: %2").arg(hibpDatabase).arg(hibpFile.errorString())
                        << endl;
        return EXIT_FAILURE;
    }

    outputTextStream << QObject::tr("Evaluating database entries against HIBP file, this will take a while...");

    QList<QPair<const Entry*, int>> findings;
    QString error;
    if (!HibpOffline::report(database, hibpFile, findings, &error)) {
        errorTextStream << error << endl;
        return EXIT_FAILURE;
    }

    for (auto& finding : findings) {
        printHibpFinding(finding.first, finding.second, outputTextStream);
    }

    return EXIT_SUCCESS;
}

void Analyze::printHibpFinding(const Entry* entry, int count, QTextStream& out)
{
    QString path = entry->title();
    for (auto g = entry->group(); g && g != g->database()->rootGroup(); g = g->parentGroup()) {
        path.prepend("/").prepend(g->name());
    }

    out << QObject::tr("Password for '%1' has been leaked %2 times!").arg(path).arg(count) << endl;
}
