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

#ifndef KEEPASSXC_TAGSTYLE_H
#define KEEPASSXC_TAGSTYLE_H

#include <QBrush>
#include <QMargins>

class QPainter;
class QPalette;
class QRectF;

/**
 * How a tag looks: a word on a rounded, filled patch. TagsEdit draws its
 * tags with this, and so does any widget showing a label in the same form.
 */
namespace TagStyle
{
    /// Space between the edge of a tag and its text.
    constexpr QMargins Margins(5, 3, 4, 3);
    constexpr int CornerRadius = 4;

    /// The fill of a tag in the given palette.
    QBrush fill(const QPalette& palette);

    /// Paints the patch of a tag; the text is left to the caller.
    void paintTag(QPainter& painter, const QRectF& rect, const QBrush& fill);
} // namespace TagStyle

#endif // KEEPASSXC_TAGSTYLE_H
