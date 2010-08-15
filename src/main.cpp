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
#include "core/Parser.h"
#include "gui/GroupModel.h"

#include "../tests/config-keepassx-tests.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Database* db= new Database();
    Parser* parser = new Parser(db);
    parser->parse(QString(KEEPASSX_TEST_DIR).append("/NewDatabase.xml"));

    GroupModel groupModel(db->rootGroup());

    QTreeView view;
    view.setModel(&groupModel);
    view.setHeaderHidden(true);
    view.expandAll();
    view.show();
    return app.exec();
}
