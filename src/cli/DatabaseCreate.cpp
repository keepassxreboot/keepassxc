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

#include "DatabaseCreate.h"

#include "Utils.h"
#include "core/Global.h"
#include "keys/FileKey.h"
#include "crypto/kdf/Argon2Kdf.h"

#include <QCommandLineParser>
#include <QFileInfo>

#define IS_ARGON2(uuid) (uuid == KeePass2::KDF_ARGON2D || uuid == KeePass2::KDF_ARGON2ID)
#define IS_AES_KDF(uuid) (uuid == KeePass2::KDF_AES_KDBX3 || uuid == KeePass2::KDF_AES_KDBX4)

const QCommandLineOption DatabaseCreate::DecryptionTimeOption =
    QCommandLineOption(QStringList() << "t" << "decryption-time",
                       QObject::tr("[Basic] Target decryption time in MS for the database."),
                       QObject::tr("time"));

const QCommandLineOption DatabaseCreate::SetKeyFileShortOption = QCommandLineOption(
    QStringList() << "k",
    QObject::tr("Set the key file for the database.\nThis option is deprecated, use --set-key-file instead."),
    QObject::tr("path"));

const QCommandLineOption DatabaseCreate::SetKeyFileOption =
    QCommandLineOption(QStringList() << "set-key-file",
                       QObject::tr("Set the key file for the database."),
                       QObject::tr("path"));

const QCommandLineOption DatabaseCreate::SetPasswordOption =
    QCommandLineOption(QStringList() << "p" << "set-password", QObject::tr("Set a password for the database."));

const QCommandLineOption DatabaseCreate::KdfOption =
    QCommandLineOption(QStringList() << "kdf",
                       QObject::tr("[Advanced] KDF algorithm for the database (argon2d, argon2id, aes-kdf)."),
                       QObject::tr("kdf"));

const QCommandLineOption DatabaseCreate::CipherOption =
    QCommandLineOption(QStringList() << "cipher",
                       QObject::tr("[Advanced] Encryption cipher (aes256, twofish, chacha20)."),
                       QObject::tr("cipher"));

const QCommandLineOption DatabaseCreate::RoundsOption =
    QCommandLineOption(QStringList() << "rounds",
                       QObject::tr("[Advanced] Number of key transform rounds."),
                       QObject::tr("rounds"));

const QCommandLineOption DatabaseCreate::MemoryOption =
    QCommandLineOption(QStringList() << "memory",
                       QObject::tr("[Advanced] [argon] Memory usage in MiB for Argon2."),
                       QObject::tr("memory"));

const QCommandLineOption DatabaseCreate::ParallelismOption =
    QCommandLineOption(QStringList() << "parallelism",
                       QObject::tr("[Advanced] [argon] Parallelism threads for Argon2."),
                       QObject::tr("parallelism"));

DatabaseCreate::DatabaseCreate()
{
    name = QString("db-create");
    description = QObject::tr("Create a new database.");
    positionalArguments.append({QString("database"), QObject::tr("Path of the database."), QString("")});
    options.append(DatabaseCreate::SetKeyFileOption);
    options.append(DatabaseCreate::SetKeyFileShortOption);
    options.append(DatabaseCreate::SetPasswordOption);
    options.append(DatabaseCreate::DecryptionTimeOption);

    options.append(DatabaseCreate::KdfOption);
    options.append(DatabaseCreate::CipherOption);
    options.append(DatabaseCreate::RoundsOption);
    options.append(DatabaseCreate::MemoryOption);
    options.append(DatabaseCreate::ParallelismOption);
}

void warnIgnored(const QString& arg, const QString& blockedBy)
{
    Utils::STDERR << QObject::tr("%1 ignored (blocked by %2)").arg(arg, blockedBy) << Qt::endl;
}

QSharedPointer<Database> DatabaseCreate::initializeDatabaseFromOptions(const QSharedPointer<QCommandLineParser>& parser)
{
    if (parser.isNull()) {
        return {};
    }

    auto& out = parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    const bool hasDecryptionTime = parser->isSet(DatabaseCreate::DecryptionTimeOption);
    const bool hasKdf            = parser->isSet(DatabaseCreate::KdfOption);
    const bool hasCipher         = parser->isSet(DatabaseCreate::CipherOption);
    const bool hasRounds         = parser->isSet(DatabaseCreate::RoundsOption);
    const bool hasMemory         = parser->isSet(DatabaseCreate::MemoryOption);
    const bool hasParallelism    = parser->isSet(DatabaseCreate::ParallelismOption);

    int decryptionTimeValue = 0;
    if (hasDecryptionTime) {
        QString val = parser->value(DatabaseCreate::DecryptionTimeOption);
        decryptionTimeValue = val.toInt();
        if (decryptionTimeValue <= 0) {
            err << QObject::tr("Invalid decryption time %1.").arg(val) << Qt::endl;
            return {};
        }
        if (decryptionTimeValue < Kdf::MIN_ENCRYPTION_TIME
            || decryptionTimeValue > Kdf::MAX_ENCRYPTION_TIME) {
            err << QObject::tr("Target decryption time must be between %1 and %2.")
                       .arg(Kdf::MIN_ENCRYPTION_TIME)
                       .arg(Kdf::MAX_ENCRYPTION_TIME)
                << Qt::endl;
            return {};
        }
    }

    auto key = QSharedPointer<CompositeKey>::create();

    if (parser->isSet(DatabaseCreate::SetPasswordOption)) {
        auto passwordKey = Utils::getConfirmedPassword();
        if (passwordKey.isNull()) {
            err << QObject::tr("Failed to set database password.") << Qt::endl;
            return {};
        }
        key->addKey(passwordKey);
    }

    if (parser->isSet(DatabaseCreate::SetKeyFileOption) || parser->isSet(DatabaseCreate::SetKeyFileShortOption)) {
        QSharedPointer<FileKey> fileKey;
        QString keyFilePath;
        if (parser->isSet(DatabaseCreate::SetKeyFileShortOption)) {
            qWarning("The -k option will be deprecated. Please use the --set-key-file option instead.");
            keyFilePath = parser->value(DatabaseCreate::SetKeyFileShortOption);
        } else {
            keyFilePath = parser->value(DatabaseCreate::SetKeyFileOption);
        }
        if (!Utils::loadFileKey(keyFilePath, fileKey)) {
            err << QObject::tr("Loading the key file failed") << Qt::endl;
            return {};
        }
        if (!fileKey.isNull()) {
            key->addKey(fileKey);
        }
    }

    if (key->isEmpty()) {
        err << QObject::tr("No key is set. Aborting database creation.") << Qt::endl;
        return {};
    }

    auto db = QSharedPointer<Database>::create();
    db->setKey(key);

    if (hasDecryptionTime) {
        if (hasKdf)         err << QObject::tr("--kdf ignored (--decryption-time)") << Qt::endl;
        if (hasCipher)      err << QObject::tr("--cipher ignored (--decryption-time)") << Qt::endl;
        if (hasRounds)      err << QObject::tr("--rounds ignored (--decryption-time)") << Qt::endl;
        if (hasMemory)      err << QObject::tr("--memory ignored (--decryption-time)") << Qt::endl;
        if (hasParallelism) err << QObject::tr("--parallelism ignored (--decryption-time)") << Qt::endl;

        auto kdf = db->kdf();
        out << QObject::tr("Benchmarking key derivation function for %1ms delay.").arg(decryptionTime) << Qt::endl;
        int rounds = kdf->benchmark(decryptionTime);
        out << QObject::tr("Setting %1 rounds for key derivation function.").arg(rounds) << Qt::endl;
        kdf->setRounds(rounds);
        if (!db->changeKdf(kdf)) {
            err << QObject::tr("Failed to set key derivation settings.") << Qt::endl;
            return {};
        }

    } else {
        // Manual mode
        if (hasCipher) {
            auto cipherUuid = KeePass2::cliStringToCipherUuid(parser->value(DatabaseCreate::CipherOption));
            if (cipherUuid.isNull()) {
                err << QObject::tr("Invalid cipher: %1").arg(parser->value(DatabaseCreate::CipherOption)) << Qt::endl;
                return {};
            }
            db->setCipher(cipherUuid);
        }

        QSharedPointer<Kdf> kdf;

        if (hasKdf) {
            auto kdfUuid = KeePass2::cliStringToKdfUuid(parser->value(DatabaseCreate::KdfOption));
            if (kdfUuid.isNull()) {
                err << QObject::tr("Invalid KDF: %1").arg(parser->value(DatabaseCreate::KdfOption)) << Qt::endl;
                return {};
            }
            kdf = KeePass2::uuidToKdf(kdfUuid);
        } else {
            kdf = db->kdf();
        }

        if (IS_AES_KDF(kdf->uuid())) {
            if (hasMemory)
                err
                    << QObject::tr("--memory ignored with %1")
                        .arg(KeePass2::kdfUuidToCliString(KeePass2::KDF_AES_KDBX4)) 
                    << Qt::endl;
            if (hasParallelism)
                err
                    << QObject::tr("--parallelism ignored with %1")
                        .arg(KeePass2::kdfUuidToCliString(KeePass2::KDF_AES_KDBX4))
                    << Qt::endl;

            if (hasRounds) {
                bool ok;
                int rounds = parser->value(DatabaseCreate::RoundsOption).toInt(&ok);
                if (!ok || rounds < 1) {
                    err << QObject::tr("Invalid rounds value: %1").arg(parser->value(DatabaseCreate::RoundsOption)) << Qt::endl;
                    return {};
                }
                if (rounds < 100000) {
                    err << QObject::tr("Rounds too low for AES-KDF, database will not be protected from brute force.") << Qt::endl;
                    return {};
                }
                kdf->setRounds(rounds);
            }
        } else if (IS_ARGON2(kdf->uuid())) {
            auto argon2 = kdf.staticCast<Argon2Kdf>();
            if (hasRounds) {
                bool ok;
                int rounds = parser->value(DatabaseCreate::RoundsOption).toInt(&ok);
                if (!ok || rounds < 1) {
                    err << QObject::tr("Invalid rounds value: %1").arg(parser->value(DatabaseCreate::RoundsOption)) << Qt::endl;
                    return {};
                }
                if (rounds > 10000) {
                    err << QObject::tr("Rounds too high for Argon2, database may take very long to open.") << Qt::endl;
                    return {};
                }
                argon2->setRounds(rounds);
            }
            if (hasMemory) {
                bool ok;
                int mib = parser->value(DatabaseCreate::MemoryOption).toInt(&ok);
                if (!ok || mib < 1) {
                    err << QObject::tr("Invalid memory value: %1").arg(parser->value(DatabaseCreate::MemoryOption)) << Qt::endl;
                    return {};
                }
                if (!argon2->setMemory(Argon2Kdf::toKibibytes(mib))) {
                    err << QObject::tr("Failed to set memory to %1 MiB, value out of range.").arg(mib) << Qt::endl;
                    return {};
                }

                argon2->setMemory(Argon2Kdf::toKibibytes(mib));
            }
            if (hasParallelism) {
                argon2->setParallelism(parser->value(DatabaseCreate::ParallelismOption).toUInt());
            }
        }

        if (!db->changeKdf(kdf)) {
            err << QObject::tr("error while setting database key derivation settings.") << Qt::endl;
            return {};
        }
    }

    return db;
}

/**
 * Create a database file using the command line. A key file and/or
 * password can be specified to encrypt the password. If none is
 * specified the function will fail.
 *
 * If a key file is specified but it can't be loaded, the function will
 * fail.
 *
 * If the database is being saved in a non existent directory, the
 * function will fail.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE on failure
 */
int DatabaseCreate::execute(const QStringList& arguments)
{
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    auto& out = parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();

    const QString& databaseFilename = args.at(0);
    if (QFileInfo::exists(databaseFilename)) {
        err << QObject::tr("File %1 already exists.").arg(databaseFilename) << Qt::endl;
        return EXIT_FAILURE;
    }

    QSharedPointer<Database> db = DatabaseCreate::initializeDatabaseFromOptions(parser);
    if (!db) {
        return EXIT_FAILURE;
    }

    QString errorMessage;
    if (!db->saveAs(databaseFilename, Database::Atomic, {}, &errorMessage)) {
        err << QObject::tr("Failed to save the database: %1.").arg(errorMessage) << Qt::endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully created new database.") << Qt::endl;
    return EXIT_SUCCESS;
}
