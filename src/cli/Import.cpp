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
#include "Import.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/Key.h"

/**
 * Create a database file from an XML export of another database.
 * A password can be specified to encrypt the database.
 * If none is specified the function will fail.
 *
 * If the database is being saved in a non existant directory, the
 * function will fail.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE on failure
 */

Import::Import()
{
    name = QString("import");
    description = QObject::tr("Import the contents of an XML database.");
    positionalArguments.append({QString("xml"), QObject::tr("Path of the XML database export."), QString("")});
    positionalArguments.append({QString("database"), QObject::tr("Path of the new database."), QString("")});
    options.append(Create::SetKeyFileOption);
    options.append(Create::SetPasswordOption);
    options.append(Create::DecryptionTimeOption);
}

int Import::execute(const QStringList& arguments)
{
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    auto& out = parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    const QString& xmlExportPath = args.at(0);
    const QString& dbPath = args.at(1);

    if (QFileInfo::exists(dbPath)) {
        err << QObject::tr("File %1 already exists.").arg(dbPath) << endl;
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

    if (parser->isSet(Create::SetPasswordOption)) {
        auto passwordKey = Utils::getConfirmedPassword();
        if (passwordKey.isNull()) {
            err << QObject::tr("Failed to set database password.") << endl;
            return EXIT_FAILURE;
        }
        key->addKey(passwordKey);
    }

    if (parser->isSet(Create::SetKeyFileOption)) {
        QSharedPointer<FileKey> fileKey;

        if (!Utils::loadFileKey(parser->value(Create::SetKeyFileOption), fileKey)) {
            err << QObject::tr("Loading the key file failed") << endl;
            return EXIT_FAILURE;
        }

        if (!fileKey.isNull()) {
            key->addKey(fileKey);
        }
    }

    if (key->isEmpty()) {
        err << QObject::tr("No key is set. Aborting database creation.") << endl;
        return EXIT_FAILURE;
    }

    QString errorMessage;
    Database db;
    db.setKey(key);

    if (decryptionTime != 0) {
        auto kdf = db.kdf();
        Q_ASSERT(kdf);

        out << QObject::tr("Benchmarking key derivation function for %1ms delay.").arg(decryptionTimeValue) << endl;
        int rounds = kdf->benchmark(decryptionTime);
        out << QObject::tr("Setting %1 rounds for key derivation function.").arg(QString::number(rounds)) << endl;
        kdf->setRounds(rounds);

        bool ok = db.changeKdf(kdf);

        if (!ok) {
            err << QObject::tr("error while setting database key derivation settings.") << endl;
            return EXIT_FAILURE;
        }
    }

    if (!db.import(xmlExportPath, &errorMessage)) {
        err << QObject::tr("Unable to import XML database: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (!db.saveAs(dbPath, &errorMessage, true, false)) {
        err << QObject::tr("Failed to save the database: %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully imported database.") << endl;
    return EXIT_SUCCESS;
}
