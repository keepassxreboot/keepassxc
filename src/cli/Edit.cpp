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

#include "Edit.h"

#include <QCommandLineParser>

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/PasswordGenerator.h"

Edit::Edit()
{
    name = QString("edit");
    description = QObject::tr("Edit an entry.");
}

Edit::~Edit()
{
}

int Edit::execute(const QStringList& arguments)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addOption(Command::QuietOption);
    parser.addOption(Command::KeyFileOption);
    parser.addOption(Command::NoPasswordOption);

    QCommandLineOption username(QStringList() << "u"
                                              << "username",
                                QObject::tr("Username for the entry."),
                                QObject::tr("username"));
    parser.addOption(username);

    QCommandLineOption url(QStringList() << "url", QObject::tr("URL for the entry."), QObject::tr("URL"));
    parser.addOption(url);

    QCommandLineOption title(QStringList() << "t"
                                           << "title",
                             QObject::tr("Title for the entry."),
                             QObject::tr("title"));
    parser.addOption(title);

    QCommandLineOption prompt(QStringList() << "p"
                                            << "password-prompt",
                              QObject::tr("Prompt for the entry's password."));
    parser.addOption(prompt);

    QCommandLineOption generate(QStringList() << "g"
                                              << "generate",
                                QObject::tr("Generate a password for the entry."));
    parser.addOption(generate);

    QCommandLineOption length(QStringList() << "l"
                                            << "password-length",
                              QObject::tr("Length for the generated password."),
                              QObject::tr("length"));
    parser.addOption(length);

    QCommandLineOption numeric(QStringList() << "n"
                                             << "numeric",
                               QObject::tr("Use numbers."));
    parser.addOption(numeric);

    QCommandLineOption special(QStringList() << "s"
                                             << "special",
                               QObject::tr("Use special characters"));
    parser.addOption(special);

    QCommandLineOption extended(QStringList() << "e"
                                              << "extended",
                                QObject::tr("Use extended ASCII"));
    parser.addOption(extended);

    QCommandLineOption exclude(QStringList() << "x"
                                             << "exclude",
                               QObject::tr("Exclude character set"),
                               QObject::tr("chars"));
    parser.addOption(exclude);

    QCommandLineOption exclude_similar(QStringList() << "exclude-similar",
                                       QObject::tr("Exclude similar looking characters"));
    parser.addOption(exclude_similar);

    QCommandLineOption every_group(QStringList() << "every-group",
                                   QObject::tr("Include characters from every selected group"));
    parser.addOption(every_group);

    parser.addPositionalArgument("entry", QObject::tr("Path of the entry to edit."));
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        errorTextStream << parser.helpText().replace("[options]", "edit [options]");
        return EXIT_FAILURE;
    }

    const QString& databasePath = args.at(0);
    const QString& entryPath = args.at(1);

    auto db = Utils::unlockDatabase(databasePath,
                                    !parser.isSet(Command::NoPasswordOption),
                                    parser.value(Command::KeyFileOption),
                                    parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT,
                                    Utils::STDERR);
    if (!db) {
        return EXIT_FAILURE;
    }

    QString passwordLength = parser.value(length);
    if (!passwordLength.isEmpty() && !passwordLength.toInt()) {
        errorTextStream << QObject::tr("Invalid value for password length: %1").arg(passwordLength) << endl;
        return EXIT_FAILURE;
    }

    Entry* entry = db->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        errorTextStream << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    if (parser.value("username").isEmpty() && parser.value("url").isEmpty() && parser.value("title").isEmpty()
        && !parser.isSet(prompt) && !parser.isSet(generate)) {
        errorTextStream << QObject::tr("Not changing any field for entry %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    entry->beginUpdate();

    if (!parser.value("title").isEmpty()) {
        entry->setTitle(parser.value("title"));
    }

    if (!parser.value("username").isEmpty()) {
        entry->setUsername(parser.value("username"));
    }

    if (!parser.value("url").isEmpty()) {
        entry->setUrl(parser.value("url"));
    }

    if (parser.isSet(prompt)) {
        if (!parser.isSet(Command::QuietOption)) {
            outputTextStream << QObject::tr("Enter new password for entry: ") << flush;
        }
        QString password = Utils::getPassword(parser.isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT);
        entry->setPassword(password);
    } else if (parser.isSet(generate)) {
        PasswordGenerator passwordGenerator;

        PasswordGenerator::CharClasses classes = 0x0;

        classes |= PasswordGenerator::DefaultCharset;

        if (parser.isSet(numeric)) {
            classes |= PasswordGenerator::Numbers;
        }
        if (parser.isSet(special)) {
            classes |= PasswordGenerator::SpecialCharacters;
        }
        if (parser.isSet(extended)) {
            classes |= PasswordGenerator::EASCII;
        }

        PasswordGenerator::GeneratorFlags flags = 0x0;

        if (parser.isSet(exclude_similar)) {
            flags |= PasswordGenerator::ExcludeLookAlike;
        }
        if (parser.isSet(every_group)) {
            flags |= PasswordGenerator::CharFromEveryGroup;
        }

        passwordGenerator.setExcludedChars(parser.value(exclude));

        if (passwordLength.isEmpty()) {
            passwordGenerator.setLength(PasswordGenerator::DefaultLength);
        } else {
            passwordGenerator.setLength(static_cast<size_t>(passwordLength.toInt()));
        }

        passwordGenerator.setCharClasses(classes);
        passwordGenerator.setFlags(flags);
        QString password = passwordGenerator.generatePassword();
        entry->setPassword(password);
    }

    entry->endUpdate();

    QString errorMessage;
    if (!db->save(databasePath, &errorMessage, true, false)) {
        errorTextStream << QObject::tr("Writing the database failed: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (!parser.isSet(Command::QuietOption)) {
        outputTextStream << QObject::tr("Successfully edited entry %1.").arg(entry->title()) << endl;
    }
    return EXIT_SUCCESS;
}
