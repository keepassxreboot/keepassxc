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

#pragma once

#include "AttachmentTypes.h"

#include <QPointer>
#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class ImageAttachmentsWidget;
}

class QGraphicsView;
class QGraphicsScene;

class ZoomHelper : public QObject
{
    Q_OBJECT
public:
    explicit ZoomHelper(double zoomFactor, double step, double min, double max, QObject* parent = nullptr);

    void zoomIn();
    void zoomOut();

    void setZoomFactor(double zoomFactor);
    double getZoomFactor() const;

    void setMinZoomOutFactor(double zoomFactor);
    void setMaxZoomInFactor(double zoomFactor);

signals:
    void zoomChanged(double zoomFactor);

private:
    double m_zoomFactor;
    double m_step;

    double m_minZoomOut;
    double m_maxZoomIn;
};

class ImageAttachmentsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageAttachmentsWidget(QWidget* parent = nullptr);
    ~ImageAttachmentsWidget() override;

    void openAttachment(attachments::Attachment attachment, attachments::OpenMode mode);
    attachments::Attachment getAttachment() const;

private slots:
    void onZoomChanged(const QString& zoomText);
    void onWheelZoomEvent(QWheelEvent* event);
    void onZoomFactorChanged(double zoomFactor);

private:
    void loadImage();

    void initZoomComboBox();

    QScopedPointer<Ui::ImageAttachmentsWidget> m_ui;
    attachments::Attachment m_attachment;

    QPointer<QGraphicsScene> m_scene;
    QPointer<ZoomHelper> m_zoomHelper;
};
