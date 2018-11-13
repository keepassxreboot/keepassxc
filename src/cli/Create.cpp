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
    description = QObject::tr("Create a new database");
}

Create::~Create()
{
}

/**
 * Create database file using the command line. A key file and/or
 * password can be specified to encrypt the password. If none is
 * specified the function will fail.
 *
 * If a key file is specified but it can't be loaded, the function will
 * fail
 *
 * If the database is being saved in a non existant directory, the
 * function will fail.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE on failure
 */
int Create::execute(const QStringList& arguments)
{
    QTextStream out(stdout, QIODevice::WriteOnly);
    QTextStream err(stdout, QIODevice::WriteOnly);

    QCommandLineParser parser;

    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    QCommandLineOption keyFile(
        QStringList() << "k" << "key-file",
        QObject::tr("Key file of the database. If file does not exist, new key file will be generated"),
        QObject::tr("path"));
    parser.addOption(keyFile);

    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli create");
        return EXIT_FAILURE;
    }

    auto key = QSharedPointer<CompositeKey>::create();

    QSharedPointer<FileKey> fileKey;

    if(parser.isSet(keyFile)) {
        if (!loadFileKey(parser.value(keyFile), fileKey)) {
            err << QObject::tr("Loading the key file failed") << endl;
            return EXIT_FAILURE;
        }
    }

    if (!fileKey.isNull()) {
        key->addKey(fileKey);
    }

    auto password = getPasswordFromStdin();
    if (!password.isNull()) {
        key->addKey(password);
    }

    if (key->isEmpty()) {
        err << QObject::tr("No key is set, aborting creating database") << endl;
        return EXIT_FAILURE;
    }

    Database db;
    db.setKey(key);

    QString dberror = db.saveToFile(args.at(0));
    if (dberror.length() > 0) {
        err << QObject::tr("Failed to save the database: ") << dberror << endl;
        return EXIT_FAILURE;
    }

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
    QTextStream out(stdout, QIODevice::WriteOnly);

    out << QObject::tr("Insert password used to encrypt database (Press enter leave blank): ");
    out.flush();
    QString password = Utils::getPassword();

    if (password.length() > 0) {
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
    QTextStream err(stdout, QIODevice::WriteOnly);

    QFileInfo fileInfo(path);
    QString error;
    fileKey = QSharedPointer<FileKey>(new FileKey());

    if (!fileInfo.exists()) {
        fileKey->create(path, &error);

        if (error.length() > 0) {
            err << QObject::tr("Creating KeyFile failed: %1").arg(error) << endl;
            return false;
        }
    }

    if (!fileKey->load(path, &error)) {
        err << QObject::tr("Loading KeyFile failed: ") << error << endl;
        return false;
    }

    return true;
}
