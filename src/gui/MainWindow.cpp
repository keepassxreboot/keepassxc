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

#include "MainWindow.h"

#include "core/Database.h"
#include "core/Metadata.h"
#include "gui/DatabaseManager.h"
#include "gui/DatabaseWidget.h"

MainWindow::MainWindow()
{
    setupUi(this);

    m_dbManager = new DatabaseManager(tabWidget);

    connect(tabWidget, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));

    connect(actionDatabaseNew, SIGNAL(triggered()), m_dbManager, SLOT(newDatabase()));
    connect(actionDatabaseOpen, SIGNAL(triggered()), m_dbManager, SLOT(openDatabase()));
    connect(actionDatabaseSave, SIGNAL(triggered()), m_dbManager, SLOT(saveDatabase()));
    connect(actionDatabaseClose, SIGNAL(triggered()), m_dbManager, SLOT(closeDatabase()));
    connect(actionQuit, SIGNAL(triggered()), SLOT(close()));
}

void MainWindow::currentTabChanged(int index)
{
    bool hasTab = (index != -1);

    actionDatabaseSave->setEnabled(hasTab);
    actionDatabaseClose->setEnabled(hasTab);
}
