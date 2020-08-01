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

struct CommandArgs
{
    CommandArgs& merge(const CommandArgs& other)
    {
        positionalArguments.append(other.positionalArguments);
        optionalArguments.append(other.optionalArguments);
        options.append(other.options);
        return *this;
    }

    QList<CommandLineArgument> positionalArguments;
    QList<CommandLineArgument> optionalArguments;
    QList<QCommandLineOption>  options;
};

class Command
{
public:
    Command(const QString& name, const QString& description)
        : m_name(name)
        , m_description(description)
    {}
    virtual ~Command() = default;

    int execute(CommandCtx& ctx, const QStringList& args);

    bool isAllowedRunmode(Runmode mode) const
    {
        return m_allowedRunmodes & runmodeMask(mode);
    }

    QString getHelpText(const CommandCtx& ctx) const;
    QString getDescriptionLine() const;

    static const QCommandLineOption HelpOption;
    static const QCommandLineOption QuietOption;

protected:
    Command(const QString& name, const QString& description, int runmodeMask)
        : Command(name, description)
    {
        m_allowedRunmodes = runmodeMask;
    }

    ///
    /// \brief Execute command with fully configured environment and parsed arguments.
    /// \param ctx configured environment ctx
    /// \param parser successfully parsed arguments and options
    /// \return EXIT_SUCCESS on success, otherwise EXIT_FAILURE
    ///
    virtual int execImpl(CommandCtx& ctx, const QCommandLineParser& parser) = 0;

    ///
    /// \brief Return positional arguments and options required for current command.
    /// \param ctx configured environment ctx
    /// \return initialized CommandArgs structure
    ///
    virtual CommandArgs getParserArgs(const CommandCtx& ctx) const { Q_UNUSED(ctx); return {}; };

    QSharedPointer<QCommandLineParser> makeParser(const CommandCtx& ctx) const;

    ///
    /// \brief Parse command arguments and return parser object with results.
    /// \param ctx configured environment ctx
    /// \param args command arguments list
    /// \return on success return valid parser object with parse results; nullptr on failure
    ///
    QSharedPointer<QCommandLineParser> parse(CommandCtx& ctx, const QStringList& args);

private:
    QString m_name;
    QString m_description;
    int m_allowedRunmodes = runmodeMaskBoth();
};

static inline const QString& getArg(int idx, Runmode currentRunmode, const QStringList& args)
{
    if (currentRunmode == Runmode::SingleCmd)
        ++idx;
    Q_ASSERT(idx < args.size());
    return args.at(idx);
}

#endif // KEEPASSXC_COMMAND_H
