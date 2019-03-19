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
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database to extract."));
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        errorTextStream << parser.helpText().replace("keepassxc-cli", "keepassxc-cli extract");
        return EXIT_FAILURE;
    }

    if (!parser.isSet(Command::QuietOption)) {
        outputTextStream << QObject::tr("Insert password to unlock %1: ").arg(args.at(0)) << flush;
    }

    auto compositeKey = QSharedPointer<CompositeKey>::create();

    QString line = Utils::getPassword(parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT);
    auto passwordKey = QSharedPointer<PasswordKey>::create();
    passwordKey->setPassword(line);
    compositeKey->addKey(passwordKey);

    QString keyFilePath = parser.value(Command::KeyFileOption);
    if (!keyFilePath.isEmpty()) {
        // LCOV_EXCL_START
        auto fileKey = QSharedPointer<FileKey>::create();
        QString errorMsg;
        if (!fileKey->load(keyFilePath, &errorMsg)) {
            errorTextStream << QObject::tr("Failed to load key file %1: %2").arg(keyFilePath, errorMsg) << endl;
            return EXIT_FAILURE;
        }

        if (fileKey->type() != FileKey::Hashed) {
            errorTextStream << QObject::tr("WARNING: You are using a legacy key file format which may become\n"
                                           "unsupported in the future.\n\n"
                                           "Please consider generating a new key file.")
                            << endl;
        }
        // LCOV_EXCL_STOP

        compositeKey->addKey(fileKey);
    }

    const QString& databaseFilename = args.at(0);
    QFile dbFile(databaseFilename);
    if (!dbFile.exists()) {
        errorTextStream << QObject::tr("File %1 does not exist.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        errorTextStream << QObject::tr("Unable to open file %1.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }

    KeePass2Reader reader;
    reader.setSaveXml(true);
    auto db = QSharedPointer<Database>::create();
    reader.readDatabase(&dbFile, compositeKey, db.data());

    QByteArray xmlData = reader.reader()->xmlData();

    if (reader.hasError()) {
        if (xmlData.isEmpty()) {
            errorTextStream << QObject::tr("Error while reading the database:\n%1").arg(reader.errorString()) << endl;
        } else {
            errorTextStream << QObject::tr("Error while parsing the database:\n%1").arg(reader.errorString()) << endl;
        }
        return EXIT_FAILURE;
    }

    outputTextStream << xmlData.constData() << endl;

    return EXIT_SUCCESS;
}
