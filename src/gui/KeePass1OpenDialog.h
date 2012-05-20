/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_KEEPASS1OPENDIALOG_H
#define KEEPASSX_KEEPASS1OPENDIALOG_H

#include "gui/DatabaseOpenDialog.h"

class KeePass1OpenDialog : public DatabaseOpenDialog
{
    Q_OBJECT

public:
    explicit KeePass1OpenDialog(QFile* file, const QString& filename, QWidget* parent = 0);
    ~KeePass1OpenDialog();

protected:
    void openDatabase();
};

#endif // KEEPASSX_KEEPASS1OPENDIALOG_H
