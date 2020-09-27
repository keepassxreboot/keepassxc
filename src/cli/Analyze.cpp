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

const QCommandLineOption Analyze::OkonOption =
    QCommandLineOption("okon",
                       QObject::tr("Path to okon-cli to search a formatted HIBP file"),
                       QObject::tr("okon-cli"));

Analyze::Analyze()
{
    name = QString("analyze");
    description = QObject::tr("Analyze passwords for weaknesses and problems.");
    options.append(Analyze::HIBPDatabaseOption);
    options.append(Analyze::OkonOption);
}

int Analyze::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    QList<QPair<const Entry*, int>> findings;
    QString error;

    auto hibpDatabase = parser->value(Analyze::HIBPDatabaseOption);
    if (!QFile::exists(hibpDatabase) || hibpDatabase.isEmpty()) {
        err << QObject::tr("Cannot find HIBP file: %1").arg(hibpDatabase);
        return EXIT_FAILURE;
    }

    auto okon = parser->value(Analyze::OkonOption);
    if (!okon.isEmpty()) {
        out << QObject::tr("Evaluating database entries using okon...") << endl;

        if (!HibpOffline::okonReport(database, okon, hibpDatabase, findings, &error)) {
            err << error << endl;
            return EXIT_FAILURE;
        }
    } else {
        QFile hibpFile(hibpDatabase);
        if (!hibpFile.open(QFile::ReadOnly)) {
            err << QObject::tr("Failed to open HIBP file %1: %2").arg(hibpDatabase).arg(hibpFile.errorString()) << endl;
            return EXIT_FAILURE;
        }

        out << QObject::tr("Evaluating database entries against HIBP file, this will take a while...") << endl;

        if (!HibpOffline::report(database, hibpFile, findings, &error)) {
            err << error << endl;
            return EXIT_FAILURE;
        }
    }

    for (auto& finding : findings) {
        printHibpFinding(finding.first, finding.second, out);
    }

    return EXIT_SUCCESS;
}

void Analyze::printHibpFinding(const Entry* entry, int count, QTextStream& out)
{
    QString path = entry->title();
    for (auto g = entry->group(); g && g != g->database()->rootGroup(); g = g->parentGroup()) {
        path.prepend("/").prepend(g->name());
    }

    if (count > 0) {
        out << QObject::tr("Password for '%1' has been leaked %2 time(s)!", "", count).arg(path).arg(count) << endl;
    } else {
        out << QObject::tr("Password for '%1' has been leaked!", "", count).arg(path) << endl;
    }
}
