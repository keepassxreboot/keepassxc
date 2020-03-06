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

#include <cstdlib>
#include <stdio.h>

#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include "Create.h"
#include "Utils.h"

#include "core/Database.h"

#include "keys/CompositeKey.h"
#include "keys/Key.h"

const QCommandLineOption Create::DecryptionTimeOption =
    QCommandLineOption(QStringList() << "t"
                                     << "decryption-time",
                       QObject::tr("Target decryption time in MS for the database."),
                       QObject::tr("time"));

Create::Create()
{
    name = QString("db-create");
    description = QObject::tr("Create a new database.");
    positionalArguments.append({QString("database"), QObject::tr("Path of the database."), QString("")});
    options.append(Command::KeyFileOption);
    options.append(Create::DecryptionTimeOption);
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
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    bool quiet = parser->isSet(Command::QuietOption);

    QTextStream out(quiet ? Utils::DEVNULL : Utils::STDOUT, QIODevice::WriteOnly);
    QTextStream err(Utils::STDERR, QIODevice::WriteOnly);

    const QStringList args = parser->positionalArguments();

    const QString& databaseFilename = args.at(0);
    if (QFileInfo::exists(databaseFilename)) {
        err << QObject::tr("File %1 already exists.").arg(databaseFilename) << endl;
        return EXIT_FAILURE;
    }

    // Validate the decryption time before asking for a password.
    QString decryptionTimeValue = parser->value(Create::DecryptionTimeOption);
    int decryptionTime = 0;
    if (decryptionTimeValue.length() != 0) {
        decryptionTime = decryptionTimeValue.toInt();
        if (decryptionTime <= 0) {
            err << QObject::tr("Invalid decryption time %1.").arg(decryptionTimeValue) << endl;
            return EXIT_FAILURE;
        }
        if (decryptionTime < Kdf::MIN_ENCRYPTION_TIME || decryptionTime > Kdf::MAX_ENCRYPTION_TIME) {
            err << QObject::tr("Target decryption time must be between %1 and %2.")
                       .arg(QString::number(Kdf::MIN_ENCRYPTION_TIME), QString::number(Kdf::MAX_ENCRYPTION_TIME))
                << endl;
            return EXIT_FAILURE;
        }
    }

    auto key = QSharedPointer<CompositeKey>::create();

    auto password = Utils::getPasswordFromStdin();
    if (!password.isNull()) {
        key->addKey(password);
    }

    QSharedPointer<FileKey> fileKey;
    if (parser->isSet(Command::KeyFileOption)) {
        if (!loadFileKey(parser->value(Command::KeyFileOption), fileKey)) {
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

    QSharedPointer<Database> db(new Database);
    db->setKey(key);
    db->setInitialized(true);

    if (decryptionTime != 0) {
        auto kdf = db->kdf();
        Q_ASSERT(kdf);

        out << QObject::tr("Benchmarking key derivation function for %1ms delay.").arg(decryptionTimeValue) << endl;
        int rounds = kdf->benchmark(decryptionTime);
        out << QObject::tr("Setting %1 rounds for key derivation function.").arg(QString::number(rounds)) << endl;
        kdf->setRounds(rounds);

        bool ok = db->changeKdf(kdf);

        if (!ok) {
            err << QObject::tr("Error while setting database key derivation settings.") << endl;
            return EXIT_FAILURE;
        }
    }

    QString errorMessage;
    if (!db->saveAs(databaseFilename, &errorMessage, true, false)) {
        err << QObject::tr("Failed to save the database: %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully created new database.") << endl;
    currentDatabase = db;
    return EXIT_SUCCESS;
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
bool Create::loadFileKey(const QString& path, QSharedPointer<FileKey>& fileKey)
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
