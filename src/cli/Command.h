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

#ifndef KEEPASSXC_COMMAND_H
#define KEEPASSXC_COMMAND_H

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "core/Database.h"
#include "CommandCtx.h"


///
/// Following traits are mandatory:
/// const char* Name        - printable command name
/// const char* Description - printable description
template<class Cmd>
struct CommandTraits;

#define DECL_TRAITS(TYPE, NAME, DESC)                       \
    template<> struct CommandTraits<TYPE> {                 \
        static constexpr const char* Name = NAME;           \
        static constexpr const char* Description = DESC;    \
    }

// At the moment, there's no QT class for the positional arguments
// like there is for the options (QCommandLineOption).
struct CommandLineArgument
{
    QString name;
    QString description;
    QString syntax;
};

class Command
{
public:
    Command() = default;
    virtual ~Command() = default;
    int execute(CommandCtx& ctx, const QStringList& arguments);

    QString name;
    QString description;

    QList<CommandLineArgument> positionalArguments;
    QList<CommandLineArgument> optionalArguments;
    QList<QCommandLineOption> options;

    QString getHelpText(const QCommandLineParser& parser) const;
    QString getDescriptionLine();

    static const QCommandLineOption HelpOption;
    static const QCommandLineOption QuietOption;

protected:
    virtual QSharedPointer<QCommandLineParser> getCommandLineParser(CommandCtx& ctx, const QStringList& arguments) = 0;
    virtual int execImpl(CommandCtx& ctx, const QCommandLineParser& parser) = 0;

    QSharedPointer<QCommandLineParser> makeParser(CommandCtx &ctx, const QStringList& args,
                                                  const QList<CommandLineArgument>& posArgs,
                                                  const QList<CommandLineArgument>& optArgs,
                                                  const QList<QCommandLineOption>& options);
};


#endif // KEEPASSXC_COMMAND_H
