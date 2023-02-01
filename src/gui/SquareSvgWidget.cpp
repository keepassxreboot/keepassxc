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

#include "SquareSvgWidget.h"
#include <QResizeEvent>

SquareSvgWidget::SquareSvgWidget(QWidget* parent)
    : QSvgWidget(parent)
{
    Q_ASSERT(parent);
    setObjectName("squareSvgWidget");
}

bool SquareSvgWidget::hasHeightForWidth() const
{
    return true;
}

int SquareSvgWidget::heightForWidth(int width) const
{
    return width;
}

// The overridden logic allows to keep the SVG image as square and centered by width and height.
void SquareSvgWidget::resizeEvent(QResizeEvent*)
{
    QWidget* pWidget = parentWidget();
    Q_ASSERT(pWidget);
    if (pWidget) {
        auto containerRect = pWidget->contentsRect();

        auto containerWidth = containerRect.width();
        auto containerHeight = containerRect.height();

        auto squareSize = qMin(containerWidth, containerHeight);
        auto halfSquareSize = squareSize >> 1;

        auto startX = (containerWidth >> 1) - halfSquareSize;
        auto startY = (containerHeight >> 1) - halfSquareSize;

        setGeometry(startX, startY, squareSize, squareSize);
    }
}
