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

#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <thread>

#include "Clip.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

const QCommandLineOption Clip::TotpOption = QCommandLineOption(QStringList() << "t"
                                                                             << "totp",
                                                               QObject::tr("Copy the current TOTP to the clipboard."));

Clip::Clip()
{
    name = QString("clip");
    description = QObject::tr("Copy an entry's password to the clipboard.");
    options.append(Clip::TotpOption);
    positionalArguments.append(
        {QString("entry"), QObject::tr("Path of the entry to clip.", "clip = copy to clipboard"), QString("")});
    optionalArguments.append(
        {QString("timeout"), QObject::tr("Timeout in seconds before clearing the clipboard."), QString("[timeout]")});
}

int Clip::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    const QStringList args = parser->positionalArguments();
    QString entryPath = args.at(1);
    QString timeout;
    if (args.size() == 3) {
        timeout = args.at(2);
    }
    bool clipTotp = parser->isSet(Clip::TotpOption);
    TextStream errorTextStream(Utils::STDERR);

    int timeoutSeconds = 0;
    if (!timeout.isEmpty() && timeout.toInt() <= 0) {
        errorTextStream << QObject::tr("Invalid timeout value %1.").arg(timeout) << endl;
        return EXIT_FAILURE;
    } else if (!timeout.isEmpty()) {
        timeoutSeconds = timeout.toInt();
    }

    TextStream outputTextStream(parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                QIODevice::WriteOnly);
    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        errorTextStream << QObject::tr("Entry %1 not found.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    QString value;
    if (clipTotp) {
        if (!entry->hasTotp()) {
            errorTextStream << QObject::tr("Entry with path %1 has no TOTP set up.").arg(entryPath) << endl;
            return EXIT_FAILURE;
        }

        value = entry->totp();
    } else {
        value = entry->password();
    }

    int exitCode = Utils::clipText(value);
    if (exitCode != EXIT_SUCCESS) {
        return exitCode;
    }

    if (clipTotp) {
        outputTextStream << QObject::tr("Entry's current TOTP copied to the clipboard!") << endl;
    } else {
        outputTextStream << QObject::tr("Entry's password copied to the clipboard!") << endl;
    }

    if (!timeoutSeconds) {
        return exitCode;
    }

    QString lastLine = "";
    while (timeoutSeconds > 0) {
        outputTextStream << '\r' << QString(lastLine.size(), ' ') << '\r';
        lastLine = QObject::tr("Clearing the clipboard in %1 second(s)...", "", timeoutSeconds).arg(timeoutSeconds);
        outputTextStream << lastLine << flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        --timeoutSeconds;
    }
    Utils::clipText("");
    outputTextStream << '\r' << QString(lastLine.size(), ' ') << '\r';
    outputTextStream << QObject::tr("Clipboard cleared!") << endl;

    return EXIT_SUCCESS;
}
