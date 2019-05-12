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

#include "Extract.h"

#include <QCommandLineParser>
#include <QFile>

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"

Extract::Extract()
{
    name = QString("extract");
    description = QObject::tr("Extract and print the content of a database.");
}

Extract::~Extract()
{
}

int Extract::execute(const QStringList& arguments)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database to extract."));
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);
    parser.addOption(Command::NoPasswordOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        errorTextStream << parser.helpText().replace("[options]", "extract [options]");
        return EXIT_FAILURE;
    }

    auto compositeKey = QSharedPointer<CompositeKey>::create();

    auto db = Utils::unlockDatabase(args.at(0),
                                    !parser.isSet(Command::NoPasswordOption),
                                    parser.value(Command::KeyFileOption),
                                    parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    QByteArray xmlData;
    QString errorMessage;
    if (!db->extract(xmlData, &errorMessage)) {
        errorTextStream << QObject::tr("Unable to extract database %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }
    outputTextStream << xmlData.constData() << endl;
    return EXIT_SUCCESS;
}
