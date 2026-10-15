/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "QrDropArea.h"

#include "gui/Icons.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>

namespace
{
    constexpr int AreaSize = 180;
    constexpr int ContentMargin = 8;
    constexpr int PlaceholderIconSize = 64;
} // namespace

QrDropArea::QrDropArea(QWidget* parent)
    : QFrame(parent)
{
    setAcceptDrops(true);
    setFixedSize(AreaSize, AreaSize);
    setToolTip(tr("Drop an image containing a TOTP QR code here"));
}

void QrDropArea::setImage(const QImage& image)
{
    m_image = image;
    update();
}

void QrDropArea::clear()
{
    m_image = {};
    update();
}

QImage QrDropArea::imageFromMimeData(const QMimeData* mimeData)
{
    if (!mimeData) {
        return {};
    }

    if (mimeData->hasImage()) {
        return qvariant_cast<QImage>(mimeData->imageData());
    }

    // Image files dragged or copied from a file manager
    for (const auto& url : mimeData->urls()) {
        if (url.isLocalFile()) {
            QImage image(url.toLocalFile());
            if (!image.isNull()) {
                return image;
            }
        }
    }

    return {};
}

void QrDropArea::dragEnterEvent(QDragEnterEvent* event)
{
    const auto* mimeData = event->mimeData();
    if (mimeData->hasImage() || mimeData->hasUrls()) {
        event->acceptProposedAction();
    }
}

void QrDropArea::dropEvent(QDropEvent* event)
{
    const auto image = imageFromMimeData(event->mimeData());
    if (image.isNull()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    setImage(image);
    emit imageDropped(image);
}

void QrDropArea::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto area = rect().adjusted(1, 1, -1, -1);
    painter.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DashLine));
    painter.setBrush(palette().color(QPalette::Base));
    painter.drawRoundedRect(area, 4, 4);

    const auto content = area.adjusted(ContentMargin, ContentMargin, -ContentMargin, -ContentMargin);

    if (!m_image.isNull()) {
        const auto scaled = m_image.scaled(content.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft(content.x() + (content.width() - scaled.width()) / 2,
                             content.y() + (content.height() - scaled.height()) / 2);
        painter.drawImage(topLeft, scaled);
        return;
    }

    const auto pixmap = icons()->icon("image-off").pixmap(PlaceholderIconSize, PlaceholderIconSize);
    const QRect iconRect(content.x() + (content.width() - PlaceholderIconSize) / 2,
                         content.y() + content.height() / 2 - PlaceholderIconSize,
                         PlaceholderIconSize,
                         PlaceholderIconSize);
    painter.drawPixmap(iconRect, pixmap);

    const QRect textRect(content.x(), iconRect.bottom() + ContentMargin, content.width(), content.height() / 2);
    painter.setPen(palette().color(QPalette::PlaceholderText));
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, tr("Drag & drop QR code"));
}
