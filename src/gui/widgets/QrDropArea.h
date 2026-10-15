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

#ifndef KEEPASSXC_QRDROPAREA_H
#define KEEPASSXC_QRDROPAREA_H

#include <QFrame>
#include <QImage>

class QMimeData;

/**
 * Square area that shows a placeholder until an image containing a QR code
 * is dropped on it or set with setImage().
 */
class QrDropArea : public QFrame
{
    Q_OBJECT

public:
    explicit QrDropArea(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void clear();

    static QImage imageFromMimeData(const QMimeData* mimeData);

signals:
    void imageDropped(const QImage& image);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_image;
};

#endif // KEEPASSXC_QRDROPAREA_H
