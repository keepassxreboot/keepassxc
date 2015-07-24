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

#ifndef KEEPASSX_UNLOCKDATABASEWIDGET_H
#define KEEPASSX_UNLOCKDATABASEWIDGET_H

#include "gui/DatabaseOpenWidget.h"

class UnlockDatabaseWidget : public DatabaseOpenWidget
{
    Q_OBJECT

public:
    explicit UnlockDatabaseWidget(QWidget* parent = nullptr);
    void clearForms();
};

#endif // KEEPASSX_UNLOCKDATABASEWIDGET_H
