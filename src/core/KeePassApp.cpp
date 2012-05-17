/*
 *  Copyright (C) 2012 Tobias Tangemann
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

#include "KeePassApp.h"

#include <QtGui/QFileOpenEvent>

KeePassApp::KeePassApp(int& argc, char** argv)
    : QApplication(argc, argv)
{
}

KeePassApp::~KeePassApp()
{
}

bool KeePassApp::event(QEvent *event)
{
    // Handle Apple QFileOpenEvent from finder (double click on .kdbx file)
    if (event->type() == QEvent::FileOpen) {
        Q_EMIT openDatabase(static_cast<QFileOpenEvent*>(event)->file());
        return true;
    }

  return (QApplication::event(event));
}
