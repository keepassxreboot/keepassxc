/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "PopupHelpWidget.h"

#include <QEvent>

#include "gui/MainWindow.h"

PopupHelpWidget::PopupHelpWidget(QWidget* parent)
    : QFrame(parent)
    , m_parentWindow(parent->window())
    , m_appWindow(getMainWindow())
    , m_offset({0, 0})
    , m_corner(Qt::BottomLeftCorner)
{
    Q_ASSERT(parent);

#ifdef Q_OS_MACOS
    setWindowFlags(Qt::FramelessWindowHint | Qt::Drawer);
#else
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
#endif
    hide();

    m_appWindow->installEventFilter(this);
    parent->installEventFilter(this);
}

PopupHelpWidget::~PopupHelpWidget()
{
    m_parentWindow->removeEventFilter(this);
    parentWidget()->removeEventFilter(this);
}

void PopupHelpWidget::setOffset(const QPoint& offset)
{
    m_offset = offset;
    if (isVisible()) {
        alignWithParent();
    }
}

void PopupHelpWidget::setPosition(Qt::Corner corner)
{
    m_corner = corner;
    if (isVisible()) {
        alignWithParent();
    }
}

bool PopupHelpWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == parentWidget() && event->type() == QEvent::FocusOut) {
        hide();
    } else if (obj == m_appWindow && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
        if (isVisible()) {
            alignWithParent();
        }
    }
    return QFrame::eventFilter(obj, event);
}

void PopupHelpWidget::showEvent(QShowEvent* event)
{
    alignWithParent();
    QFrame::showEvent(event);
}

void PopupHelpWidget::alignWithParent()
{
    QPoint pos;
    switch (m_corner) {
    case Qt::TopLeftCorner:
        pos = parentWidget()->geometry().topLeft() + m_offset - QPoint(0, height());
        break;
    case Qt::TopRightCorner:
        pos = parentWidget()->geometry().topRight() + m_offset - QPoint(width(), height());
        break;
    case Qt::BottomRightCorner:
        pos = parentWidget()->geometry().bottomRight() + m_offset - QPoint(width(), 0);
        break;
    default:
        pos = parentWidget()->geometry().bottomLeft() + m_offset;
        break;
    }

    move(m_parentWindow->mapToGlobal(pos));
}