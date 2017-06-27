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
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/PasswordGenerator.h"
#include "cli/Utils.h"

Edit::Edit()
{
    this->name = QString("edit");
    this->shellUsage = QString("edit entry_path field_name");
    this->description = QString("Edit a field from a database entry.");
}

Edit::~Edit()
{
}

int Edit::execute(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Edit a field from a database entry."));
    parser.addPositionalArgument("database", QCoreApplication::translate("main", "Path of the database."));
    parser.addPositionalArgument("entry", QCoreApplication::translate("main", "Path of the entry to add."));
    parser.addPositionalArgument("field", QCoreApplication::translate("main", "Name of the field to edit"));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 3) {
        parser.showHelp(EXIT_FAILURE);
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->editEntry(db, args.at(0), args.at(1), args.at(2));

}

int Edit::executeFromShell(Database* database, QString databasePath, QStringList arguments)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    if (arguments.size() != 2) {
        outputTextStream << this->shellUsage << "\n";
        outputTextStream.flush();
        return EXIT_FAILURE;
    }
    return this->editEntry(database, databasePath, arguments.at(0), arguments.at(1));
}

int Edit::editEntry(Database* database, QString databasePath, QString entryPath, QString fieldName)
{

    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        qCritical("Entry %s does not exist!", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    if (fieldName == "url") {

        outputTextStream << "enter new url: ";
        outputTextStream.flush();
        QString url = inputTextStream.readLine();

        entry->beginUpdate();
        entry->setUrl(url);
        entry->endUpdate();

    } else if (fieldName == "username") {

        outputTextStream << "enter new username: ";
        outputTextStream.flush();
        QString username = inputTextStream.readLine();

        entry->beginUpdate();
        entry->setUsername(username);
        entry->endUpdate();

    } else if (fieldName == "password") {

        QString password;
        if (Utils::askYesNoQuestion("Do you want to generate a new password?")) {
            PasswordGenerator passwordGenerator;
            passwordGenerator.setLength(PasswordGenerator::DefaultLength);
            passwordGenerator.setCharClasses(PasswordGenerator::LowerLetters |
                                             PasswordGenerator::UpperLetters |
                                             PasswordGenerator::Numbers);
            password = passwordGenerator.generatePassword();
        } else {
            outputTextStream << "enter new password: ";
            outputTextStream.flush();
            password = Utils::getPassword();

            outputTextStream << "confirm new password: ";
            outputTextStream.flush();
            QString passwordConfirmation = Utils::getPassword();

            if (password != passwordConfirmation) {
                qCritical("Passwords do not match.");
                return EXIT_FAILURE;
            }

        }

        entry->beginUpdate();
        entry->setPassword(password);
        entry->endUpdate();

    } else {
        qCritical("Invalid field name %s.", qPrintable(fieldName));
        return EXIT_FAILURE;
    }


    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }

    outputTextStream << "Successfully edited entry!\n";
    outputTextStream.flush();
    return EXIT_SUCCESS;

}

QStringList Edit::getSuggestions(Database* database, QStringList arguments)
{
    static QStringList fieldNames;
    if (fieldNames.isEmpty()) {
        fieldNames << "url";
        fieldNames << "password";
        fieldNames << "username";
    }

    if (arguments.size() == 1) {
        return database->rootGroup()->getSuggestions(arguments.at(0), true);
    } else if (arguments.size() == 2) {
        return fieldNames;
    }
    return QStringList();

}
