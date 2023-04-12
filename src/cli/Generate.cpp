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

#include "Generate.h"

#include "Utils.h"
#include "core/PasswordGenerator.h"

#include <QCommandLineParser>

const QCommandLineOption Generate::PasswordLengthOption =
    QCommandLineOption(QStringList() << "L"
                                     << "length",
                       QObject::tr("Length of the generated password"),
                       QObject::tr("length"));

const QCommandLineOption Generate::LowerCaseOption = QCommandLineOption(QStringList() << "l"
                                                                                      << "lower",
                                                                        QObject::tr("Use lowercase characters"));

const QCommandLineOption Generate::UpperCaseOption = QCommandLineOption(QStringList() << "U"
                                                                                      << "upper",
                                                                        QObject::tr("Use uppercase characters"));

const QCommandLineOption Generate::NumbersOption = QCommandLineOption(QStringList() << "n"
                                                                                    << "numeric",
                                                                      QObject::tr("Use numbers"));

const QCommandLineOption Generate::SpecialCharsOption = QCommandLineOption(QStringList() << "s"
                                                                                         << "special",
                                                                           QObject::tr("Use special characters"));

const QCommandLineOption Generate::ExtendedAsciiOption = QCommandLineOption(QStringList() << "e"
                                                                                          << "extended",
                                                                            QObject::tr("Use extended ASCII"));

const QCommandLineOption Generate::ExcludeCharsOption = QCommandLineOption(QStringList() << "x"
                                                                                         << "exclude",
                                                                           QObject::tr("Exclude character set"),
                                                                           QObject::tr("chars"));

const QCommandLineOption Generate::CustomCharacterSetOption =
    QCommandLineOption(QStringList() << "c"
                                     << "custom",
                       QObject::tr("Use custom character set"),
                       QObject::tr("chars"));

const QCommandLineOption Generate::ExcludeSimilarCharsOption =
    QCommandLineOption(QStringList() << "exclude-similar", QObject::tr("Exclude similar looking characters"));

const QCommandLineOption Generate::IncludeEveryGroupOption =
    QCommandLineOption(QStringList() << "every-group", QObject::tr("Include characters from every selected group"));
Generate::Generate()
{
    name = QString("generate");
    description = QObject::tr("Generate a new random password.");
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

/**
 * Creates a password generator instance using the command line options
 * of the parser object.
 */
QSharedPointer<PasswordGenerator> Generate::createGenerator(QSharedPointer<QCommandLineParser> parser)
{
    auto& err = Utils::STDERR;
    QSharedPointer<PasswordGenerator> passwordGenerator = QSharedPointer<PasswordGenerator>(new PasswordGenerator());
    QString passwordLength = parser->value(Generate::PasswordLengthOption);
    if (passwordLength.isEmpty()) {
        passwordGenerator->setLength(PasswordGenerator::DefaultLength);
    } else if (passwordLength.toInt() <= 0) {
        err << QObject::tr("Invalid password length %1").arg(passwordLength) << Qt::endl;
        return {};
    } else {
        passwordGenerator->setLength(passwordLength.toInt());
    }

    PasswordGenerator::CharClasses classes;

    if (parser->isSet(Generate::LowerCaseOption)) {
        classes |= PasswordGenerator::LowerLetters;
    }
    if (parser->isSet(Generate::UpperCaseOption)) {
        classes |= PasswordGenerator::UpperLetters;
    }
    if (parser->isSet(Generate::NumbersOption)) {
        classes |= PasswordGenerator::Numbers;
    }
    if (parser->isSet(Generate::SpecialCharsOption)) {
        classes |= PasswordGenerator::SpecialCharacters;
    }
    if (parser->isSet(Generate::ExtendedAsciiOption)) {
        classes |= PasswordGenerator::EASCII;
    }

    PasswordGenerator::GeneratorFlags flags;

    if (parser->isSet(Generate::ExcludeSimilarCharsOption)) {
        flags |= PasswordGenerator::ExcludeLookAlike;
    }
    if (parser->isSet(Generate::IncludeEveryGroupOption)) {
        flags |= PasswordGenerator::CharFromEveryGroup;
    }

    // The default charset will be used if no explicit class
    // option was set.
    if (flags != 0x0) {
        passwordGenerator->setFlags(flags);
    }
    QString customCharacterSet = parser->value(Generate::CustomCharacterSetOption);
    if (classes != 0x0 || !customCharacterSet.isNull()) {
        passwordGenerator->setCharClasses(classes);
    }
    passwordGenerator->setCustomCharacterSet(customCharacterSet);
    passwordGenerator->setExcludedCharacterSet(parser->value(Generate::ExcludeCharsOption));

    if (!passwordGenerator->isValid()) {
        err << QObject::tr("Invalid password generator after applying all options") << Qt::endl;
        return {};
    }

    return passwordGenerator;
}

int Generate::execute(const QStringList& arguments)
{
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    QSharedPointer<PasswordGenerator> passwordGenerator = Generate::createGenerator(parser);
    if (passwordGenerator.isNull()) {
        return EXIT_FAILURE;
    }

    auto& out = Utils::STDOUT;
    QString password = passwordGenerator->generatePassword();
    out << password << Qt::endl;

    return EXIT_SUCCESS;
}
