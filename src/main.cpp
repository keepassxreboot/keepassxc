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
#include "gui/csvImport/CsvImportWizard.h"
#include "gui/MessageBox.h"

#if defined(WITH_ASAN) && defined(WITH_LSAN)
#include <sanitizer/lsan_interface.h>
#endif

#ifdef QT_STATIC
#include <QtPlugin>

#ifdef Q_OS_WIN
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#elif Q_OS_LINUX
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

#ifndef QT_DEBUG
    if (app.isAlreadyRunning()) {
        qWarning() << QCoreApplication::translate("Main", "Another instance of KeePassXC is already running.").toUtf8().constData();
        return 0;
    }
#endif

    QApplication::setQuitOnLastWindowClosed(false);

    if (!Crypto::init()) {
        QString error = QCoreApplication::translate("Main",
                                                    "Fatal error while testing the cryptographic functions.");
        error.append("\n");
        error.append(Crypto::errorString());
        MessageBox::critical(nullptr, QCoreApplication::translate("Main", "KeePassXC - Error"), error);
        return 1;
    }

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

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(configOption);
    parser.addOption(keyfileOption);
    parser.addOption(pwstdinOption);

    parser.process(app);
    const QStringList args = parser.positionalArguments();

    if (parser.isSet(configOption)) {
        Config::createConfigFromFile(parser.value(configOption));
    }

    Translator::installTranslator();

#ifdef Q_OS_MAC
    // Don't show menu icons on OSX
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    MainWindow mainWindow;
    app.setMainWindow(&mainWindow);

    QObject::connect(&app, &Application::anotherInstanceStarted,
                    [&]() {
                        mainWindow.ensurePolished();
                        mainWindow.setWindowState(mainWindow.windowState() & ~Qt::WindowMinimized);
                        mainWindow.show();
                        mainWindow.raise();
                        mainWindow.activateWindow();
                    });
    QObject::connect(&app, SIGNAL(openFile(QString)), &mainWindow, SLOT(openDatabase(QString)));
    
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
        const QStringList filenames = config()->get("LastOpenedDatabases").toStringList();
        for (const QString& filename : filenames) {
            if (!filename.isEmpty() && QFile::exists(filename)) {
                mainWindow.openDatabase(filename, QString(), QString());
            }
        }
    }

    for (int ii=0; ii < args.length(); ii++) {
        QString filename = args[ii];
        if (!filename.isEmpty() && QFile::exists(filename)) {
            QString password;
            if (parser.isSet(pwstdinOption)) {
                static QTextStream in(stdin, QIODevice::ReadOnly);
                password = in.readLine();
            }
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
