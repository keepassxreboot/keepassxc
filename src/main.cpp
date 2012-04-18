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

#include <QtGui/QApplication>
#include <QtGui/QTreeView>

#include "core/Database.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2XmlReader.h"
#include "gui/MainWindow.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

#include "../tests/config-keepassx-tests.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    // don't set applicationName or organizationName as that changes QDesktopServices::storageLocation

    Crypto::init();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
