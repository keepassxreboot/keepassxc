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

#include <QFileInfo>
#include <QProcess>
#include <QScopedPointer>

namespace Utils
{
    /**
     * STDOUT file handle for the CLI.
     */
    FILE* STDOUT = stdout;

    /**
     * STDERR file handle for the CLI.
     */
    FILE* STDERR = stderr;

    /**
     * STDIN file handle for the CLI.
     */
    FILE* STDIN = stdin;

/**
 * DEVNULL file handle for the CLI.
 */
#ifdef Q_OS_WIN
    FILE* DEVNULL = fopen("nul", "w");
#else
    FILE* DEVNULL = fopen("/dev/null", "w");
#endif

    void setStdinEcho(bool enable = true)
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

    namespace Test
    {
        QStringList nextPasswords = {};

        /**
         * Set the next password returned by \link getPassword() instead of reading it from STDIN.
         * Multiple calls to this method will fill a queue of passwords.
         * This function is intended for testing purposes.
         *
         * @param password password to return next
         */
        void setNextPassword(const QString& password)
        {
            nextPasswords.append(password);
        }
    } // namespace Test

    QSharedPointer<Database> unlockDatabase(const QString& databaseFilename,
                                            const bool isPasswordProtected,
                                            const QString& keyFilename,
                                            const QString& yubiKeySlot,
                                            FILE* outputDescriptor,
                                            FILE* errorDescriptor)
    {
        auto compositeKey = QSharedPointer<CompositeKey>::create();
        TextStream out(outputDescriptor);
        TextStream err(errorDescriptor);

        QFileInfo dbFileInfo(databaseFilename);
        if (dbFileInfo.canonicalFilePath().isEmpty()) {
            err << QObject::tr("Failed to open database file %1: not found").arg(databaseFilename) << endl;
            return {};
        }

        if (!dbFileInfo.isFile()) {
            err << QObject::tr("Failed to open database file %1: not a plain file").arg(databaseFilename) << endl;
            return {};
        }

        if (!dbFileInfo.isReadable()) {
            err << QObject::tr("Failed to open database file %1: not readable").arg(databaseFilename) << endl;
            return {};
        }

        if (isPasswordProtected) {
            out << QObject::tr("Enter password to unlock %1: ").arg(databaseFilename) << flush;
            QString line = Utils::getPassword(outputDescriptor);
            auto passwordKey = QSharedPointer<PasswordKey>::create();
            passwordKey->setPassword(line);
            compositeKey->addKey(passwordKey);
        }

        if (!keyFilename.isEmpty()) {
            auto fileKey = QSharedPointer<FileKey>::create();
            QString errorMessage;
            // LCOV_EXCL_START
            if (!fileKey->load(keyFilename, &errorMessage)) {
                err << QObject::tr("Failed to load key file %1: %2").arg(keyFilename, errorMessage) << endl;
                return {};
            }

            if (fileKey->type() != FileKey::Hashed) {
                err << QObject::tr("WARNING: You are using a legacy key file format which may become\n"
                                   "unsupported in the future.\n\n"
                                   "Please consider generating a new key file.")
                    << endl;
            }
            // LCOV_EXCL_STOP

            compositeKey->addKey(fileKey);
        }

#ifdef WITH_XC_YUBIKEY
        if (!yubiKeySlot.isEmpty()) {
            bool ok = false;
            int slot = yubiKeySlot.toInt(&ok, 10);
            if (!ok || (slot != 1 && slot != 2)) {
                err << QObject::tr("Invalid YubiKey slot %1").arg(yubiKeySlot) << endl;
                return {};
            }

            QString errorMessage;
            bool blocking = YubiKey::instance()->checkSlotIsBlocking(slot, errorMessage);
            if (!errorMessage.isEmpty()) {
                err << errorMessage << endl;
                return {};
            }

            auto key = QSharedPointer<YkChallengeResponseKeyCLI>(new YkChallengeResponseKeyCLI(
                slot,
                blocking,
                QObject::tr("Please touch the button on your YubiKey to unlock %1").arg(databaseFilename),
                outputDescriptor));
            compositeKey->addChallengeResponseKey(key);
        }
#else
        Q_UNUSED(yubiKeySlot);
#endif // WITH_XC_YUBIKEY

        auto db = QSharedPointer<Database>::create();
        QString error;
        if (db->open(databaseFilename, compositeKey, &error, false)) {
            return db;
        } else {
            err << error << endl;
            return {};
        }
    }

    /**
     * Read a user password from STDIN or return a password previously
     * set by \link setNextPassword().
     *
     * @return the password
     */
    QString getPassword(FILE* outputDescriptor)
    {
        TextStream out(outputDescriptor, QIODevice::WriteOnly);

        // return preset password if one is set
        if (!Test::nextPasswords.isEmpty()) {
            auto password = Test::nextPasswords.takeFirst();
            // simulate user entering newline
            out << endl;
            return password;
        }

        static TextStream in(STDIN, QIODevice::ReadOnly);

        setStdinEcho(false);
        QString line = in.readLine();
        setStdinEcho(true);
        out << endl;

        return line;
    }

    /**
     * Read optional password from stdin.
     *
     * @return Pointer to the PasswordKey or null if passwordkey is skipped
     *         by user
     */
    QSharedPointer<PasswordKey> getPasswordFromStdin()
    {
        QSharedPointer<PasswordKey> passwordKey;
        QTextStream out(Utils::STDOUT, QIODevice::WriteOnly);

        out << QObject::tr("Enter password to encrypt database (optional): ");
        out.flush();
        QString password = Utils::getPassword();

        if (!password.isEmpty()) {
            passwordKey = QSharedPointer<PasswordKey>(new PasswordKey(password));
        }

        return passwordKey;
    }

    /**
     * A valid and running event loop is needed to use the global QClipboard,
     * so we need to use this from the CLI.
     */
    int clipText(const QString& text)
    {
        TextStream err(Utils::STDERR);

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

#ifdef Q_OS_WIN
        programName = "clip";
#endif

        if (programName.isEmpty()) {
            err << QObject::tr("No program defined for clipboard manipulation");
            err.flush();
            return EXIT_FAILURE;
        }

        QScopedPointer<QProcess> clipProcess(new QProcess(nullptr));
        clipProcess->start(programName, arguments);
        clipProcess->waitForStarted();

        if (clipProcess->state() != QProcess::Running) {
            err << QObject::tr("Unable to start program %1").arg(programName);
            err.flush();
            return EXIT_FAILURE;
        }

        if (clipProcess->write(text.toLatin1()) == -1) {
            qDebug("Unable to write to process : %s", qPrintable(clipProcess->errorString()));
        }
        clipProcess->waitForBytesWritten();
        clipProcess->closeWriteChannel();
        clipProcess->waitForFinished();

        return clipProcess->exitCode();
    }

    /**
     * Splits the given QString into a QString list. For example:
     *
     * "hello world" -> ["hello", "world"]
     * "hello    world" -> ["hello", "world"]
     * "hello\\ world" -> ["hello world"] (i.e. backslash is an escape character
     * "\"hello world\"" -> ["hello world"]
     */
    QStringList splitCommandString(const QString& command)
    {
        QStringList result;

        bool insideQuotes = false;
        QString cur;
        for (int i = 0; i < command.size(); ++i) {
            QChar c = command[i];
            if (c == '\\' && i < command.size() - 1) {
                cur.append(command[i + 1]);
                ++i;
            } else if (!insideQuotes && (c == ' ' || c == '\t')) {
                if (!cur.isEmpty()) {
                    result.append(cur);
                    cur.clear();
                }
            } else if (c == '"' && (insideQuotes || i == 0 || command[i - 1].isSpace())) {
                insideQuotes = !insideQuotes;
            } else {
                cur.append(c);
            }
        }

        if (!cur.isEmpty()) {
            result.append(cur);
        }

        return result;
    }

} // namespace Utils
