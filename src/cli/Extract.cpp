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
#include <QTextStream>

#include "cli/Utils.h"
#include "core/Database.h"
#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

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
    QTextStream out(stdout);
    QTextStream errorTextStream(stderr);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database to extract."));
    QCommandLineOption keyFile(QStringList() << "k"
                                             << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli extract");
        return EXIT_FAILURE;
    }

    out << QObject::tr("Insert password to unlock %1: ").arg(args.at(0));
    out.flush();

    CompositeKey compositeKey;

    QString line = Utils::getPassword();
    PasswordKey passwordKey;
    passwordKey.setPassword(line);
    compositeKey.addKey(passwordKey);

    QString keyFilePath = parser.value(keyFile);
    if (!keyFilePath.isEmpty()) {
        FileKey fileKey;
        QString errorMsg;
        if (!fileKey.load(keyFilePath, &errorMsg)) {
            errorTextStream << QObject::tr("Failed to load key file %1 : %2").arg(keyFilePath).arg(errorMsg);
            errorTextStream << endl;
            return EXIT_FAILURE;
        }

        if (fileKey.type() != FileKey::Hashed) {
            errorTextStream << QObject::tr("WARNING: You are using a legacy key file format which may become\n"
                                           "unsupported in the future.\n\n"
                                           "Please consider generating a new key file.");
            errorTextStream << endl;
        }

        compositeKey.addKey(fileKey);
    }

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
    Database* db = reader.readDatabase(&dbFile, compositeKey);
    delete db;

    QByteArray xmlData = reader.reader()->xmlData();

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
