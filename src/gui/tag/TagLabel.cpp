/*
 *  Copyright (C) 2026 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "TagLabel.h"

#include "gui/tag/TagStyle.h"

#include <QPainter>

TagLabel::TagLabel(QWidget* parent)
    : QLabel(parent)
    , m_fill(TagStyle::fill(palette()))
{
    setContentsMargins(TagStyle::Margins);
}

void TagLabel::setFill(const QBrush& fill)
{
    m_fill = fill;
    update();
}

void TagLabel::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    TagStyle::paintTag(painter, rect(), m_fill);
    painter.end();
    QLabel::paintEvent(event);
}
