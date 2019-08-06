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

#include "Add.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/PasswordGenerator.h"

const QCommandLineOption Add::UsernameOption = QCommandLineOption(QStringList() << "u"
                                                                                << "username",
                                                                  QObject::tr("Username for the entry."),
                                                                  QObject::tr("username"));

const QCommandLineOption Add::UrlOption =
    QCommandLineOption(QStringList() << "url", QObject::tr("URL for the entry."), QObject::tr("URL"));

const QCommandLineOption Add::PasswordPromptOption =
    QCommandLineOption(QStringList() << "p"
                                     << "password-prompt",
                       QObject::tr("Prompt for the entry's password."));

const QCommandLineOption Add::GenerateOption = QCommandLineOption(QStringList() << "g"
                                                                                << "generate",
                                                                  QObject::tr("Generate a password for the entry."));

const QCommandLineOption Add::PasswordLengthOption =
    QCommandLineOption(QStringList() << "l"
                                     << "password-length",
                       QObject::tr("Length for the generated password."),
                       QObject::tr("length"));

Add::Add()
{
    name = QString("add");
    description = QObject::tr("Add a new entry to a database.");
    options.append(Add::UsernameOption);
    options.append(Add::UrlOption);
    options.append(Add::PasswordPromptOption);
    options.append(Add::GenerateOption);
    options.append(Add::PasswordLengthOption);
    positionalArguments.append({QString("entry"), QObject::tr("Path of the entry to add."), QString("")});
}

int Add::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream inputTextStream(Utils::STDIN, QIODevice::ReadOnly);
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    const QStringList args = parser->positionalArguments();
    const QString& databasePath = args.at(0);
    const QString& entryPath = args.at(1);

    // Validating the password length here, before we actually create
    // the entry.
    QString passwordLength = parser->value(Add::PasswordLengthOption);
    if (!passwordLength.isEmpty() && !passwordLength.toInt()) {
        errorTextStream << QObject::tr("Invalid value for password length %1.").arg(passwordLength) << endl;
        return EXIT_FAILURE;
    }

    Entry* entry = database->rootGroup()->addEntryWithPath(entryPath);
    if (!entry) {
        errorTextStream << QObject::tr("Could not create entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    if (!parser->value(Add::UsernameOption).isEmpty()) {
        entry->setUsername(parser->value(Add::UsernameOption));
    }

    if (!parser->value(Add::UrlOption).isEmpty()) {
        entry->setUrl(parser->value(Add::UrlOption));
    }

    if (parser->isSet(Add::PasswordPromptOption)) {
        if (!parser->isSet(Command::QuietOption)) {
            outputTextStream << QObject::tr("Enter password for new entry: ") << flush;
        }
        QString password = Utils::getPassword(parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT);
        entry->setPassword(password);
    } else if (parser->isSet(Add::GenerateOption)) {
        PasswordGenerator passwordGenerator;

        if (passwordLength.isEmpty()) {
            passwordGenerator.setLength(PasswordGenerator::DefaultLength);
        } else {
            passwordGenerator.setLength(passwordLength.toInt());
        }

        passwordGenerator.setCharClasses(PasswordGenerator::DefaultCharset);
        passwordGenerator.setFlags(PasswordGenerator::DefaultFlags);
        QString password = passwordGenerator.generatePassword();
        entry->setPassword(password);
    }

    QString errorMessage;
    if (!database->save(databasePath, &errorMessage, true, false)) {
        errorTextStream << QObject::tr("Writing the database failed %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (!parser->isSet(Command::QuietOption)) {
        outputTextStream << QObject::tr("Successfully added entry %1.").arg(entry->title()) << endl;
    }
    return EXIT_SUCCESS;
}
