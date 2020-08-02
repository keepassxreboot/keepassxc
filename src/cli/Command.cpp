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

#include <cstdio>
#include <cstdlib>
#include <utility>

#include <QFileInfo>
#include <QMap>

#include "Command.h"

#include "Add.h"
#include "AddGroup.h"
#include "Analyze.h"
#include "Clip.h"
#include "Close.h"
#include "Create.h"
#include "Diceware.h"
#include "Edit.h"
#include "Estimate.h"
#include "Exit.h"
#include "Export.h"
#include "Generate.h"
#include "Help.h"
#include "Import.h"
#include "Info.h"
#include "List.h"
#include "Locate.h"
#include "Merge.h"
#include "Move.h"
#include "Open.h"
#include "Remove.h"
#include "RemoveGroup.h"
#include "Show.h"
#include "TextStream.h"
#include "Utils.h"


const QCommandLineOption Command::HelpOption = QCommandLineOption(QStringList()
#ifdef Q_OS_WIN
                                                                      << QStringLiteral("?")
#endif
                                                                      << QStringLiteral("h") << QStringLiteral("help"),
                                                                  QObject::tr("Display this help."));

const QCommandLineOption Command::QuietOption =
    QCommandLineOption(QStringList() << "q"
                                     << "quiet",
                       QObject::tr("Silent password prompt and other secondary outputs."));


QSharedPointer<QCommandLineParser> Command::makeParser(const CommandCtx& ctx) const
{
    auto parser = QSharedPointer<QCommandLineParser>::create();
    if (!parser)
        return QSharedPointer<QCommandLineParser>(nullptr);

    parser->setApplicationDescription(m_description);
    const CommandArgs& parserArgs = getParserArgs(ctx);
    const QList<CommandLineArgument>& posArgs = parserArgs.positionalArguments;
    const QList<CommandLineArgument>& optArgs = parserArgs.optionalArguments;
    const QList<QCommandLineOption>& options = parserArgs.options;
    for (const CommandLineArgument& arg : posArgs) {
        parser->addPositionalArgument(arg.name, arg.description, arg.syntax);
    }
    for (const CommandLineArgument& arg : optArgs) {
        parser->addPositionalArgument(arg.name, arg.description, arg.syntax);
    }
    parser->addOption(Command::HelpOption);
    parser->addOption(Command::QuietOption);
    for (const QCommandLineOption& opt : options) {
        parser->addOption(opt);
    }
    return parser;
}

QSharedPointer<QCommandLineParser> Command::parse(CommandCtx& ctx, const QStringList& args)
{
    QSharedPointer<QCommandLineParser> parser = makeParser(ctx);
    BREAK_IF(!parser, QSharedPointer<QCommandLineParser>(nullptr), ctx,
             QString("Failed to create command parser for args={ '%1' }").arg(args.join("', '")));

    BREAK_IF(!parser->parse(args), nullptr, ctx, QString("Failed to parse arguments { '%1' }: '%2'").arg(args.join("', '")).arg(parser->errorText()));

    const QStringList& parsedArgs = parser->positionalArguments();
    const CommandArgs& parserArgs = getParserArgs(ctx);
    const QList<CommandLineArgument>& posArgs = parserArgs.positionalArguments;
    const QList<CommandLineArgument>& optArgs = parserArgs.optionalArguments;
    BREAK_IF(parsedArgs.size() < posArgs.size() && !parser->isSet(HelpOption), QSharedPointer<QCommandLineParser>(nullptr),
             ctx, QString("Too few arguments.\n\n").append(getHelpText(ctx)));
    BREAK_IF(parsedArgs.size() > posArgs.size() + optArgs.size(), QSharedPointer<QCommandLineParser>(nullptr),
             ctx, QString("Too much arguments.\n\n").append(getHelpText(ctx)));

    return parser;
}

QString Command::getDescriptionLine() const
{
    QString response = m_name;
    QString space(" ");
    QString spaces = space.repeated(15 - m_name.length());
    response = response.append(spaces);
    response = response.append(m_description);
    response = response.append("\n");
    return response;
}

QString Command::getHelpText(const CommandCtx& ctx) const
{
    QString help = makeParser(ctx)->helpText();
    // Fix spacing of options parameter
    help.replace(QStringLiteral("[options]"), m_name + QStringLiteral(" [options]"));
    // Remove application directory from command line example
    QString appname = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    const QRegularExpression regex(QStringLiteral(" .*%1").arg(QRegularExpression::escape(appname)));
    help.replace(regex, appname.prepend(" "));
    return help;
}

int Command::execute(CommandCtx& ctx, const QStringList& args)
{
    QSharedPointer<QCommandLineParser> parser = parse(ctx, args);
    if (parser.isNull())
        return EXIT_FAILURE;
    if (parser->isSet(HelpOption)) {
        Utils::STDOUT << getHelpText(ctx) << endl;
        return EXIT_SUCCESS;
    }
    return execImpl(ctx, *parser);
}
