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
#include <QtGui/QApplication>
#include <QtGui/QTreeView>

#include "crypto/Crypto.h"
#include "gui/MainWindow.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    // don't set applicationName or organizationName as that changes
    // QDesktopServices::storageLocation()

    Crypto::init();

    QString filename;
    QString password;

    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); i++) {
        if (args[i] == "--password" && args.size() > (i + 1)) {
            password = args[i + 1];
            i++;
        }
        else if (!args[i].startsWith("-") && QFile::exists(args[i])) {
            filename = args[i];
        }
        else {
            qWarning("Unkown argument \"%s\"", qPrintable(args[i]));
        }
    }

    MainWindow mainWindow;
    mainWindow.show();

    if (!filename.isEmpty()) {
        mainWindow.openDatabase(filename, password, QString());
    }

    return app.exec();
}
