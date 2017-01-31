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

#include <cli/Merge.h>
#include <cli/Extract.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>

#include "config-keepassx.h"
#include "crypto/Crypto.h"

int main(int argc, char **argv)
{

    if (!Crypto::init()) {
        qFatal("Fatal error while testing the cryptographic functions:\n%s", qPrintable(Crypto::errorString()));
        return 1;
    }

    QCoreApplication app(argc, argv);
    app.setApplicationVersion(KEEPASSX_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "KeepassXC command line interface."));
    parser.addPositionalArgument("command", QCoreApplication::translate("main", "Name of the command to execute."));

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        parser.showHelp();
        return 1;
    }

    QString commandName = args.at(0);

    for (int i = 1; i < argc - 1; ++i) {
      argv[i] = argv[i+1];
    }
    argv[argc - 1] = nullptr;
    argc--;

    if (commandName == "merge")
    {
      argv[0] = const_cast<char *>("keepassxc-cli merge");
      return Merge::execute(argc, argv);
    }

    if (commandName == "extract")
    {
      argv[0] = const_cast<char *>("keepassxc-cli extract");
      return Extract::execute(argc, argv);
    }

    qCritical("Invalid command %s.", qPrintable(commandName));
    parser.showHelp();
    return 1;

}
