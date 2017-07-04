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
#include <QTextStream>

#include <cli/Command.h>
#include <cli/Utils.h>

#include "config-keepassx.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

int main(int argc, char** argv)
{
#ifdef QT_NO_DEBUG
    Tools::disableCoreDumps();
#endif

    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
        return EXIT_FAILURE;
    }

    QTextStream out(stdout);
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QCommandLineParser parser;

    QString description("KeePassXC command line interface.");
    description = description.append(QString("\n\nAvailable commands:\n"));
    for (Command* command : Command::getCommands()) {
        description = description.append(command->getDescriptionLine());
    }
    parser.setApplicationDescription(QCoreApplication::translate("main", qPrintable(description)));

    parser.addPositionalArgument("command", QCoreApplication::translate("main", "Name of the command to execute."));

    parser.addHelpOption();
    parser.addVersionOption();
    // TODO : use the setOptionsAfterPositionalArgumentsMode (Qt 5.6) function
    // when available. Until then, options passed to sub-commands won't be
    // recognized by this parser.
    parser.parse(arguments);

    if (parser.positionalArguments().size() < 1) {
        QCoreApplication app(argc, argv);
        app.setApplicationVersion(KEEPASSX_VERSION);
        if (parser.isSet("version")) {
            // Switch to parser.showVersion() when available (QT 5.4).
            out << KEEPASSX_VERSION << endl;
            return EXIT_SUCCESS;
        }
        parser.showHelp();
    }

    QString commandName = parser.positionalArguments().at(0);
    Command* command = Command::getCommand(commandName);

    if (command == nullptr) {
        qCritical("Invalid command %s.", qPrintable(commandName));
        QCoreApplication app(argc, argv);
        app.setApplicationVersion(KEEPASSX_VERSION);
        // showHelp exits the application immediately, so we need to set the
        // exit code here.
        parser.showHelp(EXIT_FAILURE);
    }

    char* commandFullName = Utils::createStringCopy(QString("keepassxc-cli ").append(commandName));

    // Removing the first cli argument before dispatching.
    ++argv;
    --argc;
    argv[0] = commandFullName;
    int exitCode = command->execute(argc, argv);
    free(commandFullName);

#if defined(WITH_ASAN) && defined(WITH_LSAN)
    // do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
    __lsan_do_leak_check();
    __lsan_disable();
#endif

    return exitCode;
}
