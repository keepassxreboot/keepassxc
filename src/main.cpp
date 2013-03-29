/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include <QtCore/QFile>

#include "core/ArgumentParser.h"
#include "core/Config.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Application.h"
#include "gui/MainWindow.h"

int main(int argc, char** argv)
{
#ifdef QT_NO_DEBUG
    Tools::disableCoreDumps();
#endif

    Application app(argc, argv);
    // don't set applicationName or organizationName as that changes
    // QDesktopServices::storageLocation()

    Crypto::init();

    const QStringList args = app.arguments();
    QHash<QString, QString> argumentMap = ArgumentParser::parseArguments(args);

    if (!argumentMap.value("config").isEmpty()) {
        Config::createConfigFromFile(argumentMap.value("config"));
    }

    MainWindow mainWindow;
    mainWindow.show();

    QObject::connect(&app, SIGNAL(openFile(QString)), &mainWindow, SLOT(openDatabase(QString)));

    QString filename(argumentMap.value("filename"));
    if (!filename.isEmpty() && QFile::exists(filename)) {
        mainWindow.openDatabase(filename, argumentMap.value("password"), QString());
    }

    return app.exec();
}
