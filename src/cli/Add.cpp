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

#include "Add.h"

#include "Generate.h"
#include "Utils.h"
#include "core/Group.h"
#include "core/PasswordGenerator.h"
#include "core/Tools.h"

#include <QCommandLineParser>

const QCommandLineOption Add::UsernameOption = QCommandLineOption(QStringList() << "u"
                                                                                << "username",
                                                                  QObject::tr("Username for the entry."),
                                                                  QObject::tr("username"));

const QCommandLineOption Add::UrlOption =
    QCommandLineOption(QStringList() << "url", QObject::tr("URL for the entry."), QObject::tr("URL"));

const QCommandLineOption Add::NotesOption =
    QCommandLineOption(QStringList() << "notes", QObject::tr("Notes for the entry."), QObject::tr("Notes"));

const QCommandLineOption Add::PasswordPromptOption =
    QCommandLineOption(QStringList() << "p"
                                     << "password-prompt",
                       QObject::tr("Prompt for the entry's password."));

const QCommandLineOption Add::GenerateOption = QCommandLineOption(QStringList() << "g"
                                                                                << "generate",
                                                                  QObject::tr("Generate a password for the entry."));

const QCommandLineOption Add::AttributeOption =
    QCommandLineOption(QStringList() << "a" << "attribute", QObject::tr("Name of the attribute for the entry."), QObject::tr("attribute"));

const QCommandLineOption Add::AttributeValueOption =
    QCommandLineOption(QStringList() << "A" << "attribute-value", QObject::tr("Value of the attribute for the entry."), QObject::tr("attribute-value"));

const QCommandLineOption Add::AttributeProtectOption =
    QCommandLineOption(QStringList() << "P" << "protect", QObject::tr("Set the attribute to be protected."));

Add::Add()
{
    name = QString("add");
    description = QObject::tr("Add a new entry to a database.");
    options.append(Add::UsernameOption);
    options.append(Add::UrlOption);
    options.append(Add::NotesOption);
    options.append(Add::PasswordPromptOption);
    options.append(Add::AttributeOption);
    options.append(Add::AttributeValueOption);
    options.append(Add::AttributeProtectOption);
    positionalArguments.append({QString("entry"), QObject::tr("Path of the entry to add."), QString("")});

    // Password generation options.
    options.append(Add::GenerateOption);
    options.append(Generate::PasswordLengthOption);
    options.append(Generate::LowerCaseOption);
    options.append(Generate::UpperCaseOption);
    options.append(Generate::NumbersOption);
    options.append(Generate::SpecialCharsOption);
    options.append(Generate::ExtendedAsciiOption);
    options.append(Generate::ExcludeCharsOption);
    options.append(Generate::ExcludeSimilarCharsOption);
    options.append(Generate::IncludeEveryGroupOption);
    options.append(Generate::CustomCharacterSetOption);
}

int Add::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    auto& entryPath = args.at(1);

    // Cannot use those 2 options at the same time!
    if (parser->isSet(Add::GenerateOption) && parser->isSet(Add::PasswordPromptOption)) {
        err << QObject::tr("Cannot generate a password and prompt at the same time.") << endl;
        return EXIT_FAILURE;
    }

    // Validating the password generator here, before we actually create
    // the entry.
    QSharedPointer<PasswordGenerator> passwordGenerator;
    if (parser->isSet(Add::GenerateOption)) {
        passwordGenerator = Generate::createGenerator(parser);
        if (passwordGenerator.isNull()) {
            return EXIT_FAILURE;
        }
    }

    Entry* entry = database->rootGroup()->addEntryWithPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not create entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    if (!parser->value(Add::UsernameOption).isEmpty()) {
        entry->setUsername(parser->value(Add::UsernameOption));
    }

    if (!parser->value(Add::UrlOption).isEmpty()) {
        entry->setUrl(parser->value(Add::UrlOption));
    }

    if (!parser->value(Add::NotesOption).isEmpty()) {
        entry->setNotes(parser->value(Add::NotesOption).replace("\\n", "\n"));
    }

    if (parser->isSet(Add::PasswordPromptOption)) {
        if (!parser->isSet(Command::QuietOption)) {
            out << QObject::tr("Enter password for new entry: ") << flush;
        }
        QString password = Utils::getPassword(parser->isSet(Command::QuietOption));
        entry->setPassword(password);
    } else if (parser->isSet(Add::GenerateOption)) {
        QString password = passwordGenerator->generatePassword();
        entry->setPassword(password);
    }

	if (!parser->value(Add::AttributeOption).isEmpty()) {

		bool attributeProtect = false;
		if(parser->isSet(Add::AttributeProtectOption)) {
			attributeProtect = true;
		}

		QString attributeValue = "";
		if(parser->isSet(Add::AttributeValueOption)) {
			QByteArray qbaAttributeValue = parser->value(Add::AttributeValueOption).toUtf8();
			if(!Tools::isBase64(qbaAttributeValue)){
        		err << QObject::tr("Attribute value is not base64 encoded.") << endl;
        		return EXIT_FAILURE;
			}else{
				attributeValue = QString( QByteArray::fromBase64(qbaAttributeValue) );
			}
		}

		entry->attributes()->set(parser->value(Add::AttributeOption),attributeValue,attributeProtect);
    }

    QString errorMessage;
    if (!database->save(Database::Atomic, {}, &errorMessage)) {
        err << QObject::tr("Writing the database failed %1.").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    if (!parser->isSet(Command::QuietOption)) {
        out << QObject::tr("Successfully added entry %1.").arg(entry->title()) << endl;
    }
    return EXIT_SUCCESS;
}
