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

#include "DragTabBar.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QTimer>

DragTabBar::DragTabBar(QWidget* parent)
    : QTabBar(parent)
    , m_tabSwitchTimer(new QTimer(this))
    , m_tabSwitchIndex(-1)
{
    m_tabSwitchTimer->setSingleShot(true);
    connect(m_tabSwitchTimer, SIGNAL(timeout()), SLOT(dragSwitchTab()));

    setAcceptDrops(true);
}

void DragTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    int tab = tabAt(event->pos());

    if (tab != -1) {
        if (tab != currentIndex()) {
            m_tabSwitchIndex = tab;
            m_tabSwitchTimer->start(QApplication::doubleClickInterval() * 2);
        }
        event->setAccepted(true);
    } else {
        QTabBar::dragEnterEvent(event);
    }
}

void DragTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    int tab = tabAt(event->pos());

    if (tab != -1) {
        if (tab == currentIndex()) {
            m_tabSwitchTimer->stop();
        } else if (tab != m_tabSwitchIndex) {
            m_tabSwitchIndex = tab;
            m_tabSwitchTimer->start(QApplication::doubleClickInterval() * 2);
        }
        event->setAccepted(true);
    } else {
        m_tabSwitchIndex = -1;
        m_tabSwitchTimer->stop();
        QTabBar::dragMoveEvent(event);
    }
}

void DragTabBar::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_tabSwitchIndex = -1;
    m_tabSwitchTimer->stop();
    QTabBar::dragLeaveEvent(event);
}

void DragTabBar::dropEvent(QDropEvent* event)
{
    m_tabSwitchIndex = -1;
    m_tabSwitchTimer->stop();
    QTabBar::dropEvent(event);
}

void DragTabBar::tabLayoutChange()
{
    m_tabSwitchIndex = -1;
    m_tabSwitchTimer->stop();
    QTabBar::tabLayoutChange();
}

void DragTabBar::dragSwitchTab()
{
    int tab = tabAt(mapFromGlobal(QCursor::pos()));

    if (tab != -1 && tab == m_tabSwitchIndex) {
        m_tabSwitchIndex = -1;
        setCurrentIndex(tab);
    }
}
