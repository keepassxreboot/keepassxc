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

#include "Open.h"

#include <QCommandLineParser>
#include <QDir>

#include "DatabaseCommand.h"
#include "TextStream.h"
#include "Utils.h"
#include "core/Metadata.h"

#include "Exit.h"

Open::Open()
{
    name = QString("open");
    description = QObject::tr("Open a database.");
}

class LineReader
{
public:
    virtual ~LineReader() = default;
    virtual QString readLine(QString prompt) = 0;
    virtual bool isFinished() = 0;
};

class SimpleLineReader : public LineReader
{
public:
    SimpleLineReader()
        : inStream(stdin, QIODevice::ReadOnly)
        , outStream(stdout, QIODevice::WriteOnly)
        , finished(false)
    {
    }

    QString readLine(QString prompt) override
    {
        outStream << prompt;
        outStream.flush();
        QString result = inStream.readLine();
        if (result.isNull()) {
            finished = true;
        }
        return result;
    }

    bool isFinished() override
    {
        return finished;
    }

private:
    TextStream inStream;
    TextStream outStream;
    bool finished;
};

#if defined(USE_READLINE)
class ReadlineLineReader : public LineReader
public:
    ReadlineLineReader()
        : finished(false)
    {
    }

    QString readLine(QString prompt) override
    {
        char* result = readline(prompt.toStdString().c_str());
        if (!result) {
            finished = true;
            return {};
        }
        add_history(result);
        QString qstr(result);
        free(result);
        return qstr;
    }

    bool isFinished() override
    {
        return finished;
    }

private:
    bool finished;
};
#endif

static int commandLoop(CommandCtx& ctx)
{
    auto& err = Utils::STDERR;
    // TODO_vanda: replace command list with interactive version
    // Commands::setupCommands(true);

    QScopedPointer<LineReader> reader(new
#if defined(USE_READLINE)
                ReadlineLineReader
#else
                SimpleLineReader
#endif
    );

    QString command;
    while (ctx.getRunmode() == Runmode::InteractiveCmd) {
        QString prompt;
        if (ctx.hasDb()) {
            const Database& db = ctx.getDb();
            prompt += db.metadata()->name();
            if (prompt.isEmpty()) {
                prompt += QFileInfo(db.filePath()).fileName();
            }
        }
        prompt += "> ";
        command = reader->readLine(prompt);
        if (reader->isFinished()) {
            break;
        }

        QStringList args = Utils::splitCommandString(command);
        if (args.empty()) {
            continue;
        }

        const QString& cmdName = args.first();
        QSharedPointer<Command> cmd = ctx.getCmd(cmdName);
        if (!cmd) {
            err << QObject::tr("Command '%1' not found.\n").arg(cmdName);
            err.flush();
            continue;
        }
        // TODO_vanda: no need to insert 'path' when the command args become
        // appropriate to current runmode
        args.insert(1, ctx.getDb().filePath());
        if (cmd->execute(ctx, args) == EXIT_FAILURE) {
            err << QObject::tr("Failed to execute command '%1'.").arg(cmdName) << endl;
            for (const auto& e : ctx.getErrors())
                err << e << endl;
            ctx.clearErrors();
        }
    }
    return ctx.error()
            ? EXIT_FAILURE
            : EXIT_SUCCESS;
}

int Open::executeWithDatabase(CommandCtx& ctx, const QCommandLineParser& parser)
{
    Q_UNUSED(parser);
    Q_ASSERT(ctx.hasDb());

    ctx.startInteractive();
    return commandLoop(ctx);
}
