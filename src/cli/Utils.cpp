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

#include "Utils.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <QProcess>
#include <QStringList>
#include <QTextStream>

void Utils::setStdinEcho(bool enable = true)
{
#ifdef Q_OS_WIN
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hIn, &mode);

    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }

    SetConsoleMode(hIn, mode);

#else
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);

    if (enable) {
        t.c_lflag |= ECHO;
    } else {
        t.c_lflag &= ~ECHO;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

QString Utils::getPassword()
{
    static QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    static QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    setStdinEcho(false);
    QString line = inputTextStream.readLine();
    setStdinEcho(true);

    // The new line was also not echoed, but we do want to echo it.
    outputTextStream << "\n";
    outputTextStream.flush();

    return line;
}

bool Utils::askYesNoQuestion(QString question, bool askContinue)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);

    if (askContinue) {
        outputTextStream << question << "\n"
                         << "Do you want to continue? [Y/n] ";
    } else {
        outputTextStream << question << " [Y/n] ";
    }
    outputTextStream.flush();

    QString answer = inputTextStream.readLine();

    if (answer.trimmed() == "y" || answer.trimmed() == "Y") {
        return true;
    }

    return false;
}

/*
 * A valid and running event loop is needed to use the global QClipboard,
 * so we need to use this from the CLI.
 */
int Utils::clipText(QString text)
{

    QString programName = "";
    QStringList arguments;

#ifdef Q_OS_UNIX
    programName = "xclip";
    arguments << "-i"
              << "-selection"
              << "clipboard";
#endif

#ifdef Q_OS_MACOS
    programName = "pbcopy";
#endif

    // TODO add this for windows.
    if (programName.isEmpty()) {
        qCritical("No program defined for clipboard manipulation");
        return EXIT_FAILURE;
    }

    QProcess* clipProcess = new QProcess(nullptr);
    clipProcess->start(programName, arguments);
    clipProcess->waitForStarted();

    const char* data = qPrintable(text);
    clipProcess->write(data, strlen(data));
    clipProcess->waitForBytesWritten();
    clipProcess->closeWriteChannel();
    clipProcess->waitForFinished();

    return clipProcess->exitCode();
}

char* Utils::createStringCopy(QString originalString)
{
    const char* original = qPrintable(originalString);
    char* response = static_cast<char*>(malloc(sizeof(char) * strlen(original)));
    strcpy(response, original);
    return response;
}
