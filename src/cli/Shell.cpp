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

#ifdef Q_OS_UNIX
#include <wordexp.h>
#endif

QStringList getArguments(QString line, bool keepEmptyParts = false)
{
    QString::SplitBehavior splitBehavior = QString::SkipEmptyParts;
    if (keepEmptyParts) {
        splitBehavior = QString::KeepEmptyParts;
    }

#ifdef Q_OS_UNIX
    wordexp_t result;
    int error = wordexp(qPrintable(line), &result, 0);
    if (error) {
        return line.split(QRegExp(" "), splitBehavior);
    }
    QStringList arguments;
    for (int i = 0; i < result.we_wordc; ++i) {
        arguments << QString(result.we_wordv[i]);
    }
    wordfree(&result);
    if (keepEmptyParts && line.endsWith(" ") && !line.endsWith("\\ ")) {
        arguments << QString("");
    }
    return arguments;
#endif
    return line.split(QRegExp(" "), splitBehavior);
}

// There must be a better way to escape this.
QString escapeForShell(QString text)
{
    // There must be other characters to escape.
    text = text.replace("\\", "\\\\");
    text = text.replace(" ", "\\ ");
    text = text.replace("\"", "\\\"");
    text = text.replace("'", "\\'");
    return text;
}

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

    QStringList arguments = getArguments(QString::fromUtf8(rl_line_buffer), true);
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
            // Each string the generator function returns as a match
            // must be allocated with malloc();
            return Utils::createStringCopy(escapeForShell(currentSuggestion));
        }
    }

    // We don't want readline to perform its default completion if this function returns no matches
    rl_attempted_completion_over = 1;
    return nullptr;

}

int charIsQuoted(char* line, int index)
{
    if (line[index] == '\t') {
        return 0;
    }
    if (index == 0) {
        return 0;
    }
    if (line[index - 1] != '\\') {
        return 0;
    }
    return !charIsQuoted(line, index - 1);
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
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
#ifdef WITH_XC_READLINE
    char* chars = readline(qPrintable(prompt));
    // This happens if readline encounters an EOF while reading the line.
    // We want to print the next prompt on a new line.
    if (chars == nullptr) {
        outputTextStream << endl;
    }
    QString line(chars);

    if (!line.isEmpty()) {
        add_history(chars);
    }
#else
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
    // Only allow quoting with the double quotes.
    rl_completer_quote_characters = "\"";
    rl_completer_word_break_characters = " \t\"\'";
    rl_char_is_quoted_p = &charIsQuoted;
#endif

    while (true) {

      out.flush();
      QString line = getLine("KeePassXC> ");
      if (line.isEmpty()) {
          continue;
      }

      QStringList arguments = getArguments(line.trimmed());
      if (arguments.isEmpty()) {
          continue;
      }

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
