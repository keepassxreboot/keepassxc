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

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>

#include "cli/TextStream.h"
#include <cli/Command.h>

#include "config-keepassx.h"
#include "core/Bootstrap.h"
#include "crypto/Crypto.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

int main(int argc, char** argv)
{
    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(KEEPASSXC_VERSION);

#ifdef QT_NO_DEBUG
    Bootstrap::bootstrapApplication();
#endif

    TextStream out(stdout);
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QCommandLineParser parser;

    QString description("KeePassXC command line interface.");
    description = description.append(QObject::tr("\n\nAvailable commands:\n"));
    for (Command* command : Command::getCommands()) {
        description = description.append(command->getDescriptionLine());
    }
    parser.setApplicationDescription(description);

    parser.addPositionalArgument("command", QObject::tr("Name of the command to execute."));

    parser.addHelpOption();
    parser.addVersionOption();
    // TODO : use the setOptionsAfterPositionalArgumentsMode (Qt 5.6) function
    // when available. Until then, options passed to sub-commands won't be
    // recognized by this parser.
    parser.parse(arguments);

    if (parser.positionalArguments().empty()) {
        if (parser.isSet("version")) {
            // Switch to parser.showVersion() when available (QT 5.4).
            out << KEEPASSXC_VERSION << endl;
            return EXIT_SUCCESS;
        }
        parser.showHelp();
    }

    QString commandName = parser.positionalArguments().at(0);
    Command* command = Command::getCommand(commandName);

    if (command == nullptr) {
        qCritical("Invalid command %s.", qPrintable(commandName));
        // showHelp exits the application immediately, so we need to set the
        // exit code here.
        parser.showHelp(EXIT_FAILURE);
    }

    // Removing the first argument (keepassxc).
    arguments.removeFirst();
    int exitCode = command->execute(arguments);

#if defined(WITH_ASAN) && defined(WITH_LSAN)
    // do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
    __lsan_do_leak_check();
    __lsan_disable();
#endif

    return exitCode;
}
