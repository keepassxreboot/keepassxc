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
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "cli/Utils.h"
#include "core/Database.h"
#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"

Extract::Extract()
{
    this->name = QString("extract");
    this->description = QString("Extract and print the content of a database.");
}

Extract::~Extract()
{
}

int Extract::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "Extract and print the content of a database."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database to extract."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        parser.showHelp(EXIT_FAILURE);
    }

    out << "Insert the database password\n> ";
    out.flush();

    QString line = Utils::getPassword();
    CompositeKey key = CompositeKey::readFromLine(line);

    QString databaseFilename = args.at(0);
    QFile dbFile(databaseFilename);
    if (!dbFile.exists()) {
        qCritical("File %s does not exist.", qPrintable(databaseFilename));
        return EXIT_FAILURE;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open file %s.", qPrintable(databaseFilename));
        return EXIT_FAILURE;
    }

    KeePass2Reader reader;
    reader.setSaveXml(true);
    Database* db = reader.readDatabase(&dbFile, key);
    delete db;

    QByteArray xmlData = reader.xmlData();

    if (reader.hasError()) {
        if (xmlData.isEmpty()) {
            qCritical("Error while reading the database:\n%s", qPrintable(reader.errorString()));
        } else {
            qWarning("Error while parsing the database:\n%s\n", qPrintable(reader.errorString()));
        }
        return EXIT_FAILURE;
    }

    out << xmlData.constData() << "\n";

    return EXIT_SUCCESS;
}
