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
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream err(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database to extract."));
    QCommandLineOption keyFile(QStringList() << "k" << "key-file",
                               QObject::tr("Key file of the database."),
                               QObject::tr("path"));
    parser.addOption(keyFile);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli extract");
        return EXIT_FAILURE;
    }

    out << QObject::tr("Insert password to unlock %1: ").arg(args.at(0)) << flush;

    auto compositeKey = QSharedPointer<CompositeKey>::create();

    QString line = Utils::getPassword();
    auto passwordKey = QSharedPointer<PasswordKey>::create();
    passwordKey->setPassword(line);
    compositeKey->addKey(passwordKey);

    QString keyFilePath = parser.value(keyFile);
    if (!keyFilePath.isEmpty()) {
        // LCOV_EXCL_START
        auto fileKey = QSharedPointer<FileKey>::create();
        QString errorMsg;
        if (!fileKey->load(keyFilePath, &errorMsg)) {
            err << QObject::tr("Failed to load key file %1: %2").arg(keyFilePath).arg(errorMsg) << endl;
            return EXIT_FAILURE;
        }

        if (fileKey->type() != FileKey::Hashed) {
            err << QObject::tr("WARNING: You are using a legacy key file format which may become\n"
                               "unsupported in the future.\n\n"
                               "Please consider generating a new key file.") << endl;
        }
        // LCOV_EXCL_STOP

        compositeKey->addKey(fileKey);
    }

    const QString& databaseFilename = args.at(0);
    QFile dbFile(databaseFilename);
    if (!dbFile.exists()) {
        err << QObject::tr("File %1 does not exist.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        err << QObject::tr("Unable to open file %1.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }

    KeePass2Reader reader;
    reader.setSaveXml(true);
    QScopedPointer<Database> db(reader.readDatabase(&dbFile, compositeKey));

    QByteArray xmlData = reader.reader()->xmlData();

    if (reader.hasError()) {
        if (xmlData.isEmpty()) {
            err << QObject::tr("Error while reading the database:\n%1").arg(reader.errorString()) << endl;
        } else {
            err << QObject::tr("Error while parsing the database:\n%1").arg(reader.errorString()) << endl;
        }
        return EXIT_FAILURE;
    }

    out << xmlData.constData() << endl;

    return EXIT_SUCCESS;
}
