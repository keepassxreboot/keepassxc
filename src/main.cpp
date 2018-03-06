/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Tools.h"
#include "core/Translator.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

#ifdef QT_STATIC
#include <QtPlugin>

#if defined(Q_OS_WIN)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
#endif
#endif

int main(int argc, char** argv)
{
#ifdef QT_NO_DEBUG
    Tools::disableCoreDumps();
#endif
    Tools::setupSearchPaths();

    Application app(argc, argv);
    Application::setApplicationName("keepassxc");
    Application::setApplicationVersion(KEEPASSX_VERSION);
    // don't set organizationName as that changes the return value of
    // QStandardPaths::writableLocation(QDesktopServices::DataLocation)

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "KeePassXC - cross-platform password manager"));
    parser.addPositionalArgument("filename", QCoreApplication::translate("main", "filenames of the password databases to open (*.kdbx)"), "[filename(s)]");

    QCommandLineOption configOption("config",
                                    QCoreApplication::translate("main", "path to a custom config file"),
                                    "config");
    QCommandLineOption keyfileOption("keyfile",
                                     QCoreApplication::translate("main", "key file of the database"),
                                     "keyfile");
    QCommandLineOption pwstdinOption("pw-stdin",
                                     QCoreApplication::translate("main", "read password of the database from stdin"));
    // This is needed under Windows where clients send --parent-window parameter with Native Messaging connect method
    QCommandLineOption parentWindowOption(QStringList() << "pw"
                                                        << "parent-window",
                                                        QCoreApplication::translate("main", "Parent window handle"),
                                                        "handle");

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(configOption);
    parser.addOption(keyfileOption);
    parser.addOption(pwstdinOption);
    parser.addOption(parentWindowOption);

    parser.process(app);
    const QStringList fileNames = parser.positionalArguments();

    if (app.isAlreadyRunning()) {
        if (!fileNames.isEmpty()) {
            app.sendFileNamesToRunningInstance(fileNames);
        }
        qWarning() << QCoreApplication::translate("Main", "Another instance of KeePassXC is already running.").toUtf8().constData();
        return 0;
    }

    QApplication::setQuitOnLastWindowClosed(false);

    if (!Crypto::init()) {
        QString error = QCoreApplication::translate("Main",
                                                    "Fatal error while testing the cryptographic functions.");
        error.append("\n");
        error.append(Crypto::errorString());
        MessageBox::critical(nullptr, QCoreApplication::translate("Main", "KeePassXC - Error"), error);
        return 1;
    }

    if (parser.isSet(configOption)) {
        Config::createConfigFromFile(parser.value(configOption));
    }

    Translator::installTranslators();

#ifdef Q_OS_MAC
    // Don't show menu icons on OSX
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    MainWindow mainWindow;
    app.setMainWindow(&mainWindow);

    QObject::connect(&app, SIGNAL(anotherInstanceStarted()), &mainWindow, SLOT(bringToFront()));
    QObject::connect(&app, SIGNAL(applicationActivated()), &mainWindow, SLOT(bringToFront()));
    QObject::connect(&app, SIGNAL(openFile(QString)), &mainWindow, SLOT(openDatabase(QString)));
    QObject::connect(&app, SIGNAL(quitSignalReceived()), &mainWindow, SLOT(appExit()), Qt::DirectConnection);

    // start minimized if configured
    bool minimizeOnStartup = config()->get("GUI/MinimizeOnStartup").toBool();
    bool minimizeToTray    = config()->get("GUI/MinimizeToTray").toBool();
    if (minimizeOnStartup) {
        mainWindow.setWindowState(Qt::WindowMinimized);
    }
    if (!(minimizeOnStartup && minimizeToTray)) {
        mainWindow.show();
    }

    if (config()->get("OpenPreviousDatabasesOnStartup").toBool()) {
        const QStringList fileNames = config()->get("LastOpenedDatabases").toStringList();
        for (const QString& filename: fileNames) {
            if (!filename.isEmpty() && QFile::exists(filename)) {
                mainWindow.openDatabase(filename);
            }
        }
    }

    const bool pwstdin = parser.isSet(pwstdinOption);
    for (const QString& filename: fileNames) {
        QString password;
        if (pwstdin) {
            // we always need consume a line of STDIN if --pw-stdin is set to clear out the
            // buffer for native messaging, even if the specified file does not exist
            static QTextStream in(stdin, QIODevice::ReadOnly);
            password = in.readLine();
        }

        if (!filename.isEmpty() && QFile::exists(filename) && !filename.endsWith(".json", Qt::CaseInsensitive)) {
            mainWindow.openDatabase(filename, password, parser.value(keyfileOption));
        }
    }

    int exitCode = app.exec();

#if defined(WITH_ASAN) && defined(WITH_LSAN)
    // do leak check here to prevent massive tail of end-of-process leak errors from third-party libraries
    __lsan_do_leak_check();
    __lsan_disable();
#endif

    return exitCode;
}
