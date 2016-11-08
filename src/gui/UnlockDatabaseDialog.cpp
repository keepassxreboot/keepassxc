/*
 *  Copyright (C) 2016 KeePassXC Team
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

#include "UnlockDatabaseDialog.h"
#include "UnlockDatabaseWidget.h"

#include "autotype/AutoType.h"
#include "gui/DragTabBar.h"
#include "core/Database.h"


UnlockDatabaseDialog::UnlockDatabaseDialog(QWidget *parent)
    : QDialog(parent)
    , m_view(new UnlockDatabaseWidget(this))
{
    connect(m_view, SIGNAL(editFinished(bool)), this, SLOT(complete(bool)));
}

void UnlockDatabaseDialog::setDBFilename(const QString &filename)
{
    m_view->load(filename);
}

void UnlockDatabaseDialog::clearForms()
{
    m_view->clearForms();
}

Database *UnlockDatabaseDialog::database()
{
    return m_view->database();
}

void UnlockDatabaseDialog::complete(bool r)
{
    if (r) {
        accept();
        Q_EMIT unlockDone(true);
    } else {
        reject();
    }
}
