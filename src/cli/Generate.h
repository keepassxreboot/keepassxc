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

#ifndef KEEPASSXC_GENERATE_H
#define KEEPASSXC_GENERATE_H

#include "Command.h"

#include "core/PasswordGenerator.h"


/**
 * Creates a password generator instance using the command line options
 * of the parser object.
 */
QSharedPointer<PasswordGenerator> createGenerator(CommandCtx& ctx, const QCommandLineParser& parser);

class Generate final : public Command
{
    using Ancestor = Command;
public:
    using Ancestor::Ancestor;

    static const QCommandLineOption PasswordLengthOption;
    static const QCommandLineOption LowerCaseOption;
    static const QCommandLineOption UpperCaseOption;
    static const QCommandLineOption NumbersOption;
    static const QCommandLineOption SpecialCharsOption;
    static const QCommandLineOption ExtendedAsciiOption;
    static const QCommandLineOption ExcludeCharsOption;
    static const QCommandLineOption ExcludeSimilarCharsOption;
    static const QCommandLineOption IncludeEveryGroupOption;

private:
    CommandArgs getParserArgs(const CommandCtx& ctx) const override;
    int execImpl(CommandCtx& ctx, const QCommandLineParser& parser) override;
};
DECL_TRAITS(Generate, "generate", "Generate a new random password.");

#endif // KEEPASSXC_GENERATE_H
