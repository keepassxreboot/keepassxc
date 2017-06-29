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
#include "Shell.h"
#include "Command.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "cli/Utils.h"
#include "config-keepassx.h"

#ifdef WITH_XC_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

Shell::Shell()
{
    this->name = QString("shell");
    this->description = QString("Launch the interactive shell.");
}

Shell::~Shell()
{
}

Database* database;


#ifdef WITH_XC_READLINE

// Each string the generator function returns as a match
// must be allocated with malloc();
char* createStringCopy(QString originalString)
{
    const char* original = qPrintable(originalString);
    char* response = static_cast<char*>(malloc(sizeof(char) * strlen(original)));
    strcpy(response, original);
    return response;
}

char* commandArgumentsCompletion(const char*, int state)
{

    static int currentIndex;
    static QStringList commandNames;
    static QStringList allCommandNames;

    if (commandNames.isEmpty()) {
        for (Command* command : Command::getShellCommands()) {
            commandNames << command->name;
            allCommandNames << command->name;
        }
        allCommandNames << QString("quit");
        allCommandNames << QString("help");
    }

    if (state == 0) {
        currentIndex = 0;
    }

    QStringList arguments = QString::fromUtf8(rl_line_buffer).split(QRegExp(" "), QString::KeepEmptyParts);
    QString currentText = arguments.last();
    QString commandName = arguments.takeFirst();

    QStringList suggestions;
    rl_completion_suppress_append = 1;

    if (arguments.size() == 0) {
        rl_completion_suppress_append = 0;
        suggestions = allCommandNames;
    } else {

        Command* command = Command::getCommand(commandName);
        if (commandName == "help" && arguments.size() == 1) {
            suggestions = commandNames;
        } else if (command) {
            suggestions = command->getSuggestions(database, arguments);
        }
    }

    while (currentIndex < suggestions.size()) {
        QString currentSuggestion = suggestions.at(currentIndex++);
        if (currentSuggestion.startsWith(currentText)) {
            return createStringCopy(currentSuggestion);
        }
    }

    return nullptr;

}

char** shellCompletion(const char* text, int, int)
{
    return rl_completion_matches(text, commandArgumentsCompletion);
}
#endif


void printHelp()
{
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    for (Command* command : Command::getShellCommands()) {
        outputTextStream << command->getDescriptionLine();
    }
    outputTextStream.flush();
}

QString getLine(QString prompt)
{
#ifdef WITH_XC_READLINE
    char* chars = readline(qPrintable(prompt));
    QString line(chars);

    if (!line.isEmpty()) {
      add_history(chars);
    }
#else
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    outputTextStream << prompt;
    outputTextStream.flush();

    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QString line = inputTextStream.readLine();
#endif
    return line;
}

int Shell::execute(int argc, char** argv)
{
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString(argv[i]);
    }
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Start KeePassXC's shell"));
    parser.addPositionalArgument("database",
                                 QCoreApplication::translate("main", "Path of the database."));
    QCommandLineOption guiPrompt(
        QStringList() << "g"
                      << "gui-prompt",
        QCoreApplication::translate("main", "Use a GUI prompt unlocking the database."));
    parser.addOption(guiPrompt);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 1) {
        QCoreApplication app(argc, argv);
        parser.showHelp(EXIT_FAILURE);
    }

    QCoreApplication app(argc, argv);
    database = Database::unlockFromStdin(args.at(0));

    if (!database) {
        return EXIT_FAILURE;
    }

    out << "KeePassXC " << KEEPASSX_VERSION << " interactive shell\n";
    if (!database->metadata()->name().isNull()) {
        out << "Using database " << database->metadata()->name() << endl;
    } else {
        out << "Using database " << args.at(0) << endl;
    }
    out << "Use 'help' to list the available commands." << endl;

#ifdef WITH_XC_READLINE
    rl_readline_name = const_cast<char*>("kpxcli");
    rl_attempted_completion_function = shellCompletion;
#endif

    while (true) {

      out.flush();
      QString line = getLine("KeePassXC> ");
      if (line.isEmpty()) {
          continue;
      }

      QStringList arguments = line.trimmed().split(QRegExp(" "));
      QString commandName = arguments.takeFirst();

      Command* command = Command::getCommand(commandName);

      if (command && command->isShellCommand()) {
          command->executeFromShell(database, args.at(0), arguments);
      } else if (commandName == QString("help")) {
          if (arguments.size() == 0) {
              printHelp();
              continue;
          }
          QString helpCommandName = arguments.at(0);
          Command* helpCommand = Command::getCommand(helpCommandName);
          if (!helpCommand) {
              out << QString("Invalid command '" + commandName + "'\n");
              continue;
          }
          out << helpCommand->getShellUsageLine() << endl;
      } else if (commandName == QString("quit")) {
          break;
      } else {
          out << QString("Invalid command '" + commandName + "'\n");
      }

    }

    delete database;
    return EXIT_SUCCESS;
}
