/*
 *  Copyright (C) 2007 Trolltech ASA <info@trolltech.com>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Florian Geyer <blueice@fobos.de>
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

#include "LineEdit.h"

#include <QStyle>
#include <QToolButton>

#include "core/FilePath.h"

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_clearButton(new QToolButton(this))
{
    m_clearButton->setObjectName("clearButton");

    QIcon icon;
    QString iconNameDirected =
        QString("edit-clear-locationbar-").append((layoutDirection() == Qt::LeftToRight) ? "rtl" : "ltr");
    icon = QIcon::fromTheme(iconNameDirected);
    if (icon.isNull()) {
        icon = QIcon::fromTheme("edit-clear");
        if (icon.isNull()) {
            icon = filePath()->icon("actions", iconNameDirected, false);
        }
    }

    m_clearButton->setIcon(icon);
    m_clearButton->setCursor(Qt::ArrowCursor);
    m_clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    m_clearButton->hide();
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(updateCloseButton(QString)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(
        QString("QLineEdit { padding-right: %1px; } ").arg(m_clearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), m_clearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), m_clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEdit::resizeEvent(QResizeEvent* event)
{
    QSize sz = m_clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int y = (rect().bottom() + 1 - sz.height()) / 2;

    if (layoutDirection() == Qt::LeftToRight) {
        m_clearButton->move(rect().right() - frameWidth - sz.width(), y);
    } else {
        m_clearButton->move(rect().left() + frameWidth, y);
    }

    QLineEdit::resizeEvent(event);
}

void LineEdit::updateCloseButton(const QString& text)
{
    m_clearButton->setVisible(!text.isEmpty());
}
