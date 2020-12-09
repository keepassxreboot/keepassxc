/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_ROWBUTTONHELPER_H
#define KEEPASSXC_FDOSECRETS_ROWBUTTONHELPER_H

#include <functional>

class QAbstractItemView;
class QWidget;
class QModelIndex;

void installWidgetItemDelegate(QAbstractItemView* view,
                               int column,
                               std::function<QWidget*(QWidget*, const QModelIndex&)>&& create);

/**
 * @brief Open an editor on the cell, the editor's user property will be edited.
 */
template <typename Editor>
void installWidgetItemDelegate(QAbstractItemView* view,
                               int column,
                               std::function<Editor*(QWidget*, const QModelIndex&)>&& create)
{
    installWidgetItemDelegate(view, column, [create](QWidget* p, const QModelIndex& idx) { return create(p, idx); });
}

#endif // KEEPASSXC_FDOSECRETS_ROWBUTTONHELPER_H
