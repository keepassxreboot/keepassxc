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

#include <QApplication>

#include "gui/MainWindow.h"

PopupHelpWidget::PopupHelpWidget(QWidget* parent)
    : QFrame(parent)
    , m_appWindow(getMainWindow())
    , m_offset({0, 0})
    , m_corner(Qt::BottomLeftCorner)
{
    Q_ASSERT(parent);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    hide();

    m_appWindow->installEventFilter(this);
    parentWidget()->installEventFilter(this);
}

PopupHelpWidget::~PopupHelpWidget()
{
    m_appWindow->removeEventFilter(this);
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
    if (isVisible()) {
        if (obj == parentWidget() && event->type() == QEvent::FocusOut && qApp->focusWindow() != windowHandle()) {
            hide();
        } else if (obj == m_appWindow && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
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
    QPoint pos = m_offset;
    switch (m_corner) {
    case Qt::TopLeftCorner:
        pos += QPoint(0, -height());
        break;
    case Qt::TopRightCorner:
        pos += QPoint(parentWidget()->width(), -height());
        break;
    case Qt::BottomRightCorner:
        pos += QPoint(parentWidget()->width(), parentWidget()->height());
        break;
    case Qt::BottomLeftCorner:
    default:
        pos += QPoint(0, parentWidget()->height());
        break;
    }

    move(parentWidget()->mapToGlobal(pos));
}
