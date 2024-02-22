/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CustomTableWidget.h"

CustomTableWidget::CustomTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
}

void CustomTableWidget::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && !selectedItems().isEmpty()) {
        emit acceptSelections();
    } else {
        QTableView::keyPressEvent(event);
    }
}

void CustomTableWidget::focusInEvent(QFocusEvent* event)
{
    // For some reason accept button gets selected if table is clicked without any
    // selections, even if the button is actually disabled. Connecting to this
    // signal and adjusting the button focuses fixes the issue.
    if (event->reason() == Qt::MouseFocusReason && selectedItems().isEmpty()) {
        emit focusInWithoutSelections();
    }
}
