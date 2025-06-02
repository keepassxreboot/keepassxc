/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "ImageAttachmentsView.h"

#include <QDebug>
#include <QWheelEvent>

#include <limits>

ImageAttachmentsView::ImageAttachmentsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void ImageAttachmentsView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        emit ctrlWheelEvent(event);
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void ImageAttachmentsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if (m_autoFitInView) {
        fitSceneInView();
    }
}

void ImageAttachmentsView::showEvent(QShowEvent* event)
{
    if (m_autoFitInView) {
        fitSceneInView();
    }

    QGraphicsView::showEvent(event);
}

void ImageAttachmentsView::fitSceneInView()
{
    if (auto scene = ImageAttachmentsView::scene()) {
        ImageAttachmentsView::fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void ImageAttachmentsView::enableAutoFitInView()
{
    m_autoFitInView = true;
    fitSceneInView();
}

void ImageAttachmentsView::disableAutoFitInView()
{
    m_autoFitInView = false;
}

bool ImageAttachmentsView::isAutoFitInViewActivated() const
{
    return m_autoFitInView;
}

double ImageAttachmentsView::calculateFitInViewFactor() const
{
    auto viewPort = viewport();
    if (auto currentScene = scene(); currentScene && viewPort) {
        const auto itemsRect = currentScene->itemsBoundingRect().size();

        // If the image rect is empty
        if (itemsRect.isEmpty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        const auto viewPortSize = viewPort->size();
        // Calculate the zoom factor based on the current size and the image rect
        return std::min(viewPortSize.width() / itemsRect.width(), viewPortSize.height() / itemsRect.height());
    }

    return std::numeric_limits<double>::quiet_NaN();
}
