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

#include <cli/Clip.h>
#include <cli/EntropyMeter.h>
#include <cli/Extract.h>
#include <cli/List.h>
#include <cli/Merge.h>
#include <cli/Show.h>

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

    QCommandLineParser parser;

    QString description("KeePassXC command line interface.");
    description = description.append(QString("\n\nAvailable commands:"));
    description = description.append(QString("\n  clip\t\tCopy a password to the clipboard."));
    description = description.append(QString("\n  extract\tExtract and print the content of a database."));
    description = description.append(QString("\n  entropy-meter\tCalculate password entropy."));
    description = description.append(QString("\n  list\t\tList database entries."));
    description = description.append(QString("\n  merge\t\tMerge two databases."));
    description = description.append(QString("\n  show\t\tShow a password."));
    parser.setApplicationDescription(QCoreApplication::translate("main", qPrintable(description)));

    parser.addPositionalArgument("command", QCoreApplication::translate("main", "Name of the command to execute."));

    parser.addHelpOption();
    parser.addVersionOption();
    // TODO : use process once the setOptionsAfterPositionalArgumentsMode (Qt 5.6)
    // is available. Until then, options passed to sub-commands won't be
    // recognized by this parser.
    // parser.process(app);

    if (argc < 2) {
        QCoreApplication app(argc, argv);
        app.setApplicationVersion(KEEPASSX_VERSION);
        parser.showHelp();
        return EXIT_FAILURE;
    }

    QString commandName = argv[1];

    // Removing the first cli argument before dispatching.
    ++argv;
    --argc;

    int exitCode = EXIT_FAILURE;

    if (commandName == "clip") {
        argv[0] = const_cast<char*>("keepassxc-cli clip");
        exitCode = Clip::execute(argc, argv);
    } else if (commandName == "entropy-meter") {
        argv[0] = const_cast<char*>("keepassxc-cli entropy-meter");
        exitCode = EntropyMeter::execute(argc, argv);
    } else if (commandName == "extract") {
        argv[0] = const_cast<char*>("keepassxc-cli extract");
        exitCode = Extract::execute(argc, argv);
    } else if (commandName == "list") {
        argv[0] = const_cast<char*>("keepassxc-cli list");
        exitCode = List::execute(argc, argv);
    } else if (commandName == "merge") {
        argv[0] = const_cast<char*>("keepassxc-cli merge");
        exitCode = Merge::execute(argc, argv);
    } else if (commandName == "show") {
        argv[0] = const_cast<char*>("keepassxc-cli show");
        exitCode = Show::execute(argc, argv);
    } else {
        qCritical("Invalid command %s.", qPrintable(commandName));
        QCoreApplication app(argc, argv);
        app.setApplicationVersion(KEEPASSX_VERSION);
        parser.showHelp();
        exitCode = EXIT_FAILURE;
    }

#if defined(WITH_ASAN) && defined(WITH_LSAN)
    // do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
    __lsan_do_leak_check();
    __lsan_disable();
#endif

    return exitCode;
}
