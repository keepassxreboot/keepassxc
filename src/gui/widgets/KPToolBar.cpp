/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "KPToolBar.h"

#include <QAbstractButton>
#include <QAction>
#include <QEvent>
#include <QLayout>
#include <QToolButton>

KPToolBar::KPToolBar(const QString& title, QWidget* parent)
    : QToolBar(title, parent)
{
    init();
}

KPToolBar::KPToolBar(QWidget* parent)
    : QToolBar(parent)
{
    init();
}

void KPToolBar::init()
{
    m_expandButton = findChild<QAbstractButton*>("qt_toolbar_ext_button");
    m_expandTimer.setSingleShot(true);
    connect(&m_expandTimer, &QTimer::timeout, this, [this] { setExpanded(false); });
}

bool KPToolBar::isExpanded()
{
    return !canExpand() || (canExpand() && m_expandButton->isChecked());
}

bool KPToolBar::canExpand()
{
    return m_expandButton && m_expandButton->isVisible();
}

void KPToolBar::updateButtonAccessibility()
{
    for (QAction* action : actions()) {
        if (action->isSeparator()) {
            continue;
        }
        QWidget* widget = widgetForAction(action);
        if (auto* button = qobject_cast<QToolButton*>(widget)) {
            const QString name = action->toolTip().isEmpty()
                ? action->text().remove(QLatin1Char('&'))
                : action->toolTip();
            if (!name.isEmpty()) {
                button->setAccessibleName(name);
            }
            button->setFocusPolicy(Qt::TabFocus);
        }
    }
}

void KPToolBar::setExpanded(bool state)
{
    if (canExpand() && !QMetaObject::invokeMethod(layout(), "setExpanded", Q_ARG(bool, state))) {
        qWarning("Toolbar: Cannot invoke setExpanded!");
    }
}

bool KPToolBar::event(QEvent* event)
{
    // Override events handled by the base class for better UX when using an expandable toolbar.
    switch (event->type()) {
    case QEvent::Leave:
        // Hide the toolbar after 2 seconds of mouse exit
        m_expandTimer.start(2000);
        return true;
    case QEvent::Enter:
        // Mouse came back in, stop hiding timer
        m_expandTimer.stop();
        return true;
    case QEvent::Show:
    case QEvent::LayoutRequest:
        updateButtonAccessibility();
        break;
    default:
        break;
    }
    return QToolBar::event(event);
}
