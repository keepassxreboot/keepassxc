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

#include "TagStyle.h"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>

QBrush TagStyle::fill(const QPalette& palette)
{
    return palette.brush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight);
}

void TagStyle::paintTag(QPainter& painter, const QRectF& rect, const QBrush& fill)
{
    QPainterPath path;
    path.addRoundedRect(rect, CornerRadius, CornerRadius);
    painter.fillPath(path, fill);
}
