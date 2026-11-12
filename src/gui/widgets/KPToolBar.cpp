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

#include "gui/Application.h"

#include <QAbstractButton>
#include <QEvent>
#include <QLayout>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

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

void KPToolBar::setExpanded(bool state)
{
    if (canExpand() && !QMetaObject::invokeMethod(layout(), "setExpanded", Q_ARG(bool, state))) {
        qWarning("Toolbar: Cannot invoke setExpanded!");
    }
}

KPToolBar::CloudState KPToolBar::cloudState() const
{
    return m_cloudState;
}

void KPToolBar::setCloudState(KPToolBar::CloudState state)
{
    if (m_cloudState != state) {
        m_cloudState = state;
        update();
    }
}

void KPToolBar::paintEvent(QPaintEvent* event)
{
    // Draw the regular toolbar first; child widgets (buttons, search) are painted on top of this.
    QToolBar::paintEvent(event);

    if (m_cloudState == CloudState::None) {
        return;
    }

    const bool dark = kpxcApp->isDarkTheme();
    const bool online = m_cloudState == CloudState::Online;

    QColor transparent;
    QColor tint;
    QColor cloudColor;
    if (online) {
        tint = dark ? QColor(30, 70, 140, 242) : QColor(74, 144, 217, 217);
        cloudColor = dark ? QColor(207, 224, 255, 140) : QColor(255, 255, 255, 242);
    } else {
        tint = dark ? QColor(110, 80, 30, 242) : QColor(160, 150, 140, 217);
        cloudColor = dark ? QColor(240, 220, 176, 128) : QColor(255, 255, 255, 230);
    }
    transparent = tint;
    transparent.setAlpha(0);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(rect());

    // Left side stays untouched, fade to the tint towards the right
    QLinearGradient gradient(width() * 0.35, 0, width(), 0);
    gradient.setColorAt(0, transparent);
    gradient.setColorAt(1, tint);
    painter.fillRect(rect(), gradient);

    // The cloud is about twice the toolbar height and intentionally clipped by the toolbar edges
    const qreal cloudHeight = height() * 1.9;
    const qreal cloudWidth = cloudHeight * 200.0 / 120.0;
    const qreal x = width() - cloudWidth - 8;
    const qreal y = -height() * 0.48;
    const qreal sx = cloudWidth / 200.0;
    const qreal sy = cloudHeight / 120.0;

    QPainterPath cloud;
    cloud.moveTo(50, 108);
    cloud.cubicTo(22, 108, 6, 90, 6, 70);
    cloud.cubicTo(6, 52, 20, 40, 38, 40);
    cloud.cubicTo(42, 18, 62, 4, 86, 4);
    cloud.cubicTo(112, 4, 130, 20, 134, 40);
    cloud.cubicTo(158, 36, 194, 50, 194, 78);
    cloud.cubicTo(194, 96, 178, 108, 156, 108);
    cloud.closeSubpath();

    painter.save();
    painter.translate(x, y);
    painter.scale(sx, sy);
    painter.fillPath(cloud, cloudColor);
    painter.restore();

    if (!online) {
        // Slash through the cloud to signal that the original file is unavailable
        QPen pen(dark ? QColor(255, 138, 122) : QColor(192, 57, 43));
        pen.setWidthF(qMax(3.0, height() * 0.11));
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(QPointF(x + cloudWidth * 0.18, height() * 0.95),
                         QPointF(x + cloudWidth * 0.82, height() * 0.08));
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
    default:
        return QToolBar::event(event);
    }
}
