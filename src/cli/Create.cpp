/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include <QCommandLineParser>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include "Create.h"
#include "Utils.h"

#include "core/Database.h"

#include "keys/CompositeKey.h"
#include "keys/Key.h"

Create::Create()
{
    name = QString("create");
    description = QObject::tr("Create a new database.");
}

Create::~Create()
{
}

/**
 * Create a database file using the command line. A key file and/or
 * password can be specified to encrypt the password. If none is
 * specified the function will fail.
 *
 * If a key file is specified but it can't be loaded, the function will
 * fail.
 *
 * If the database is being saved in a non existant directory, the
 * function will fail.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE on failure
 */
int Create::execute(const QStringList& arguments)
{
    QTextStream out(Utils::STDOUT, QIODevice::WriteOnly);
    QTextStream err(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;

    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addOption(Command::KeyFileOption);

    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli create");
        return EXIT_FAILURE;
    }

    QString databaseFilename = args.at(0);
    if (QFileInfo::exists(databaseFilename)) {
        err << QObject::tr("File %1 already exists.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }

    auto key = QSharedPointer<CompositeKey>::create();

    auto password = getPasswordFromStdin();
    if (!password.isNull()) {
        key->addKey(password);
    }

    QSharedPointer<FileKey> fileKey;
    if (parser.isSet(Command::KeyFileOption)) {
        if (!loadFileKey(parser.value(Command::KeyFileOption), fileKey)) {
            err << QObject::tr("Loading the key file failed") << endl;
            return EXIT_FAILURE;
        }
    }

    if (!fileKey.isNull()) {
        key->addKey(fileKey);
    }

    if (key->isEmpty()) {
        err << QObject::tr("No key is set. Aborting database creation.") << endl;
        return EXIT_FAILURE;
    }

    Database db;
    db.setKey(key);

    QString errorMessage;
    if (!db.save(databaseFilename, &errorMessage, true, false)) {
        err << QObject::tr("Failed to save the database: %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully created new database.") << endl;
    return EXIT_SUCCESS;
}

/**
 * Read optional password from stdin.
 *
 * @return Pointer to the PasswordKey or null if passwordkey is skipped
 *         by user
 */
QSharedPointer<PasswordKey> Create::getPasswordFromStdin()
{
    QSharedPointer<PasswordKey> passwordKey;
    QTextStream out(Utils::STDOUT, QIODevice::WriteOnly);

    out << QObject::tr("Insert password to encrypt database (Press enter to leave blank): ");
    out.flush();
    QString password = Utils::getPassword();

    if (!password.isEmpty()) {
        passwordKey = QSharedPointer<PasswordKey>(new PasswordKey(password));
    }

    return passwordKey;
}

/**
 * Load a key file from disk. When the path specified does not exist a
 * new file will be generated. No folders will be generated so the parent
 * folder of the specified file nees to exist
 *
 * If the key file cannot be loaded or created the function will fail.
 *
 * @param path Path to the key file to be loaded
 * @param fileKey Resulting fileKey
 * @return true if the key file was loaded succesfully
 */
bool Create::loadFileKey(QString path, QSharedPointer<FileKey>& fileKey)
{
    QTextStream err(Utils::STDERR, QIODevice::WriteOnly);

    QString error;
    fileKey = QSharedPointer<FileKey>(new FileKey());

    if (!QFileInfo::exists(path)) {
        fileKey->create(path, &error);

        if (!error.isEmpty()) {
            err << QObject::tr("Creating KeyFile %1 failed: %2").arg(path, error) << endl;
            return false;
        }
    }

    if (!fileKey->load(path, &error)) {
        err << QObject::tr("Loading KeyFile %1 failed: %2").arg(path, error) << endl;
        return false;
    }

    return true;
}
