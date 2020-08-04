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
#include <memory>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QScopedPointer>
#include <QStringList>

#include "cli/TextStream.h"
#include <cli/Command.h>

#include "DatabaseCommand.h"
#include "Open.h"
#include "Utils.h"
#include "config-keepassx.h"
#include "core/Bootstrap.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "CommandCtx.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

#if defined(USE_READLINE)
#include <readline/history.h>
#include <readline/readline.h>
#endif


static int run(const QStringList& args)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    QCommandLineParser parser;
    CommandCtx ctx(parser, args);
    if (ctx.error()) {
        err << "Failed to parse command:\n";
        for (const auto& e : ctx.getErrors()) {
            err << e << endl;
        }
        return EXIT_FAILURE;
    }

    switch (ctx.getRunmode()) {
        case Runmode::Version:
            out << KEEPASSXC_VERSION << endl;
            return EXIT_SUCCESS;
        case Runmode::DebugInfo:
            Utils::debugInfo(out);
            return EXIT_SUCCESS;
        case Runmode::Help:
            // cannot use 'showHelp' because of the asan postprocessing
            out << parser.helpText() << endl;
            return EXIT_SUCCESS;
        default: {
            QStringList cmdArgs = args;
            cmdArgs.removeFirst();
            const QString cmdName = cmdArgs.first();
            QSharedPointer<Command> cmd = ctx.getCmd(cmdName);
            if (!cmd) {
                err << QObject::tr("Command '%1' not found.").arg(cmdName) << endl;
                return EXIT_FAILURE;
            }
            const int result = cmd->execute(ctx, cmdArgs);
            if (result == EXIT_FAILURE && ctx.error()) {
                for (const auto& e : ctx.getErrors()) {
                    err << e << endl;
                }
            }
            return result;
        }
    }
}

int main(int argc, char** argv)
{
    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(KEEPASSXC_VERSION);

    Bootstrap::bootstrap();
    Utils::setDefaultTextStreams();

    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    const int exitCode = run(args);

#if defined(WITH_ASAN) && defined(WITH_LSAN)
    // do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
    __lsan_do_leak_check();
    __lsan_disable();
#endif

    return exitCode;
}
