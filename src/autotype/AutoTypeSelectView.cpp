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

#include "AutoTypeSelectView.h"

#include <QKeyEvent>
#include <QMouseEvent>

AutoTypeSelectView::AutoTypeSelectView(QWidget* parent)
    : AutoTypeMatchView(parent)
{
    setMouseTracking(true);
    setAllColumnsShowFocus(true);

    connect(model(), SIGNAL(modelReset()), SLOT(selectFirstMatch()));
}

void AutoTypeSelectView::mouseMoveEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        setCurrentIndex(index);
        setCursor(Qt::PointingHandCursor);
    } else {
        unsetCursor();
    }

    AutoTypeMatchView::mouseMoveEvent(event);
}

void AutoTypeSelectView::selectFirstMatch()
{
    QModelIndex index = model()->index(0, 0);

    if (index.isValid()) {
        setCurrentIndex(index);
    }
}

void AutoTypeSelectView::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        emit rejected();
    } else {
        e->ignore();
    }
}