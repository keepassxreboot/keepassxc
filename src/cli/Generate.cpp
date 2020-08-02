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

#include "Generate.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"


QSharedPointer<PasswordGenerator> createGenerator(CommandCtx& ctx, const QCommandLineParser& parser)
{
    auto passwordGenerator = QSharedPointer<PasswordGenerator>::create();
    const QString& passwordLength = parser.value(Generate::PasswordLengthOption);
    if (passwordLength.isEmpty())
        passwordGenerator->setLength(PasswordGenerator::DefaultLength);

    const int length = passwordLength.toInt();
    BREAK_IF(length <= 0, QSharedPointer<PasswordGenerator>(nullptr),
             ctx, QObject::tr("Invalid password length=%1").arg(passwordLength));
    passwordGenerator->setLength(length);

    PasswordGenerator::CharClasses classes = 0x0;

    if (parser.isSet(Generate::LowerCaseOption)) {
        classes |= PasswordGenerator::LowerLetters;
    }
    if (parser.isSet(Generate::UpperCaseOption)) {
        classes |= PasswordGenerator::UpperLetters;
    }
    if (parser.isSet(Generate::NumbersOption)) {
        classes |= PasswordGenerator::Numbers;
    }
    if (parser.isSet(Generate::SpecialCharsOption)) {
        classes |= PasswordGenerator::SpecialCharacters;
    }
    if (parser.isSet(Generate::ExtendedAsciiOption)) {
        classes |= PasswordGenerator::EASCII;
    }

    PasswordGenerator::GeneratorFlags flags = 0x0;

    if (parser.isSet(Generate::ExcludeSimilarCharsOption)) {
        flags |= PasswordGenerator::ExcludeLookAlike;
    }
    if (parser.isSet(Generate::IncludeEveryGroupOption)) {
        flags |= PasswordGenerator::CharFromEveryGroup;
    }

    // The default charset will be used if no explicit class
    // option was set.
    passwordGenerator->setCharClasses(classes);
    passwordGenerator->setFlags(flags);
    passwordGenerator->setExcludedChars(parser.value(Generate::ExcludeCharsOption));

    BREAK_IF(!passwordGenerator->isValid(), QSharedPointer<PasswordGenerator>(nullptr),
             ctx, QObject::tr("Invalid password generator after applying all options"));
    return passwordGenerator;
}


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

const QCommandLineOption Generate::ExcludeSimilarCharsOption =
    QCommandLineOption(QStringList() << "exclude-similar", QObject::tr("Exclude similar looking characters"));

const QCommandLineOption Generate::IncludeEveryGroupOption =
    QCommandLineOption(QStringList() << "every-group", QObject::tr("Include characters from every selected group"));


CommandArgs Generate::getParserArgs(const CommandCtx& ctx) const
{
    Q_UNUSED(ctx);
    static const CommandArgs args {
        {},
        {},
        {
            Generate::PasswordLengthOption,
            Generate::LowerCaseOption,
            Generate::UpperCaseOption,
            Generate::NumbersOption,
            Generate::SpecialCharsOption,
            Generate::ExtendedAsciiOption,
            Generate::ExcludeCharsOption,
            Generate::ExcludeSimilarCharsOption,
            Generate::IncludeEveryGroupOption
        }
    };
    return args;
}

int Generate::execImpl(CommandCtx& ctx, const QCommandLineParser& parser)
{
    Q_UNUSED(ctx);
    QSharedPointer<PasswordGenerator> passwordGenerator = createGenerator(ctx, parser);
    BREAK_IF(passwordGenerator.isNull(), EXIT_FAILURE,
             ctx, QObject::tr("Failed to generate password"));
    Utils::STDOUT << passwordGenerator->generatePassword() << endl;
    Utils::STDOUT.flush();
    return EXIT_SUCCESS;
}
