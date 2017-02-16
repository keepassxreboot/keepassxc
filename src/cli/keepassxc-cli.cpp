/*
 *  Copyright (C) 2017 KeePassXC Team
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

#include <cli/List.h>
#include <cli/Merge.h>
#include <cli/Extract.h>
#include <cli/EntropyMeter.h>
#include "config-keepassx.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"

int main(int argc, char **argv)
{
#ifdef QT_NO_DEBUG
    Tools::disableCoreDumps();
#endif

    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);
    app.setApplicationVersion(KEEPASSX_VERSION);

    QCommandLineParser parser;

    QString description("KeePassXC command line interface.");
    description = description.append(QString("\n\nAvailable commands:"));
    description = description.append(QString("\n  extract\tExtract and print the content of a database."));
    description = description.append(QString("\n  entropy-meter\tCalculate password entropy."));
    description = description.append(QString("\n  list\t\tList database entries."));
    description = description.append(QString("\n  merge\t\tMerge two databases."));
    parser.setApplicationDescription(QCoreApplication::translate("main", qPrintable(description)));

    parser.addPositionalArgument("command", QCoreApplication::translate("main", "Name of the command to execute."));

    parser.addHelpOption();
    parser.addVersionOption();
    // TODO : use process once the setOptionsAfterPositionalArgumentsMode (Qt 5.6)
    // is available. Until then, options passed to sub-commands won't be
    // recognized by this parser.
    // parser.process(app);

    if (argc < 2) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    QString commandName = argv[1];

    // Removing the first cli argument before dispatching.
    ++argv;
    --argc;

    if (commandName == "list") {
        argv[0] = const_cast<char*>("keepassxc-cli list");
        return List::execute(argc, argv);
    }

    if (commandName == "merge") {
        argv[0] = const_cast<char*>("keepassxc-cli merge");
        return Merge::execute(argc, argv);
    }

    if (commandName == "extract") {
        argv[0] = const_cast<char*>("keepassxc-cli extract");
        return Extract::execute(argc, argv);
    }

    if (commandName == "entropy-meter") {
        argv[0] = const_cast<char*>("keepassxc-cli entropy-meter");
        return EntropyMeter::execute(argc, argv);
    }

    qCritical("Invalid command %s.", qPrintable(commandName));
    parser.showHelp();
    return EXIT_FAILURE;

}
