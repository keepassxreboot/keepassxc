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

#include "PasswordInput.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <QTextStream>


PasswordInput::PasswordInput()
{
}

void PasswordInput::setStdinEcho(bool enable = true)
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

QString PasswordInput::getPassword()
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
