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

#include "WindowSelectComboBox.h"

#include <QLineEdit>

#include "autotype/AutoType.h"

WindowSelectComboBox::WindowSelectComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::ComboBox));

    // first item is always the current content of the line edit
    insertItem(0, "");
}

void WindowSelectComboBox::refreshWindowList()
{
    model()->setData(model()->index(0, 0), lineEdit()->text());

    while (count() > 1) {
        removeItem(1);
    }
    insertItems(1, autoType()->windowTitles());
}

void WindowSelectComboBox::showPopup()
{
    if (lineEdit()->isReadOnly()) {
        return;
    }

    refreshWindowList();

    QComboBox::showPopup();
}

QSize WindowSelectComboBox::sizeHint() const
{
    QSize size = lineEdit()->sizeHint();
    size.setHeight(qMax(size.height(), QComboBox::sizeHint().height()));
    return size;
}

QSize WindowSelectComboBox::minimumSizeHint() const
{
    QSize size = lineEdit()->minimumSizeHint();
    size.setHeight(qMax(size.height(), QComboBox::minimumSizeHint().height()));
    return size;
}
