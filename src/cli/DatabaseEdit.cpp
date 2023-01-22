/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseEdit.h"

#include "Utils.h"
#include "cli/DatabaseCreate.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

#include <QCommandLineParser>
#include <QFileInfo>

const QCommandLineOption DatabaseEdit::UnsetPasswordOption =
    QCommandLineOption(QStringList() << "unset-password", QObject::tr("Unset the password for the database."));
const QCommandLineOption DatabaseEdit::UnsetKeyFileOption =
    QCommandLineOption(QStringList() << "unset-key-file", QObject::tr("Unset the key file for the database."));

DatabaseEdit::DatabaseEdit()
{
    name = QString("db-edit");
    description = QObject::tr("Edit a database.");
    options.append(DatabaseCreate::SetKeyFileOption);
    options.append(DatabaseCreate::SetPasswordOption);
    options.append(DatabaseEdit::UnsetKeyFileOption);
    options.append(DatabaseEdit::UnsetPasswordOption);
}

int DatabaseEdit::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    bool databaseWasChanged = false;

    if (parser->isSet(DatabaseCreate::SetPasswordOption) && parser->isSet(DatabaseEdit::UnsetPasswordOption)) {
        err << QObject::tr("Cannot use %1 and %2 at the same time.")
                   .arg(DatabaseCreate::SetPasswordOption.names().at(0))
                   .arg(DatabaseEdit::UnsetPasswordOption.names().at(0))
            << endl;
        return EXIT_FAILURE;
    }

    if (parser->isSet(DatabaseCreate::SetKeyFileOption) && parser->isSet(DatabaseEdit::UnsetKeyFileOption)) {
        err << QObject::tr("Cannot use %1 and %2 at the same time.")
                   .arg(DatabaseCreate::SetKeyFileOption.names().at(0))
                   .arg(DatabaseEdit::UnsetKeyFileOption.names().at(0))
            << endl;
        return EXIT_FAILURE;
    }

    bool hasKeyChange =
        (parser->isSet(DatabaseCreate::SetPasswordOption) || parser->isSet(DatabaseCreate::SetKeyFileOption)
         || parser->isSet(DatabaseEdit::UnsetPasswordOption) || parser->isSet(DatabaseEdit::UnsetKeyFileOption));

    if (hasKeyChange) {
        auto newDatabaseKey = getNewDatabaseKey(database,
                                                parser->isSet(DatabaseCreate::SetPasswordOption),
                                                parser->isSet(DatabaseEdit::UnsetPasswordOption),
                                                parser->value(DatabaseCreate::SetKeyFileOption),
                                                parser->isSet(DatabaseEdit::UnsetKeyFileOption));
        if (newDatabaseKey.isNull()) {
            err << QObject::tr("Could not change the database key.") << endl;
            return EXIT_FAILURE;
        }
        database->setKey(newDatabaseKey);
        databaseWasChanged = true;
    }

    if (!databaseWasChanged) {
        out << QObject::tr("Database was not modified.") << endl;
        return EXIT_SUCCESS;
    }

    QString errorMessage;
    if (!database->save(Database::Atomic, {}, &errorMessage)) {
        err << QObject::tr("Writing the database failed: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully edited the database.") << endl;
    return EXIT_SUCCESS;
}

QSharedPointer<CompositeKey> DatabaseEdit::getNewDatabaseKey(QSharedPointer<Database> database,
                                                             bool updatePassword,
                                                             bool removePassword,
                                                             QString newFileKeyPath,
                                                             bool removeKeyFile)
{
    auto& err = Utils::STDERR;
    auto newDatabaseKey = QSharedPointer<CompositeKey>::create();
    bool updateKeyFile = !newFileKeyPath.isEmpty();

    auto currentPasswordKey = database->key()->getKey(PasswordKey::UUID);
    auto currentFileKey = database->key()->getKey(FileKey::UUID);
    auto currentChallengeResponseKey = database->key()->getChallengeResponseKey(ChallengeResponseKey::UUID);

    if (removePassword && currentPasswordKey.isNull()) {
        err << QObject::tr("Cannot remove password: The database does not have a password.") << endl;
        return {};
    }

    if (removeKeyFile && currentFileKey.isNull()) {
        err << QObject::tr("Cannot remove file key: The database does not have a file key.") << endl;
        return {};
    }

    if (updatePassword) {
        QSharedPointer<PasswordKey> newPasswordKey = Utils::getConfirmedPassword();
        if (newPasswordKey.isNull()) {
            err << QObject::tr("Failed to set database password.") << endl;
            return {};
        }
        newDatabaseKey->addKey(newPasswordKey);
    } else if (!removePassword && !currentPasswordKey.isNull()) {
        newDatabaseKey->addKey(currentPasswordKey);
    }

    if (updateKeyFile) {
        QSharedPointer<FileKey> newFileKey = QSharedPointer<FileKey>::create();
        QString errorMessage;
        if (!Utils::loadFileKey(newFileKeyPath, newFileKey)) {
            err << QObject::tr("Loading the new key file failed: %1").arg(errorMessage) << endl;
            return {};
        }
        newDatabaseKey->addKey(newFileKey);
    } else if (!removeKeyFile && !currentFileKey.isNull()) {
        newDatabaseKey->addKey(currentFileKey);
    }

    // This is a sanity check to make sure that this function is not used if
    // new key types are introduced. Otherwise, those key types would be
    // silently removed from the database.
    for (const QSharedPointer<Key>& key : database->key()->keys()) {
        if (key->uuid() != PasswordKey::UUID && key->uuid() != FileKey::UUID) {
            err << QObject::tr("Found unexpected Key type %1").arg(key->uuid().toString()) << endl;
            return {};
        }
    }
    for (const QSharedPointer<ChallengeResponseKey>& key : database->key()->challengeResponseKeys()) {
        if (key->uuid() != ChallengeResponseKey::UUID) {
            err << QObject::tr("Found unexpected Key type %1").arg(key->uuid().toString()) << endl;
            return {};
        }
    }

    if (!currentChallengeResponseKey.isNull()) {
        newDatabaseKey->addChallengeResponseKey(currentChallengeResponseKey);
    }

    if (newDatabaseKey->keys().isEmpty() && newDatabaseKey->challengeResponseKeys().isEmpty()) {
        err << QObject::tr("Cannot remove all the keys from a database.") << endl;
        return {};
    }

    return newDatabaseKey;
}
