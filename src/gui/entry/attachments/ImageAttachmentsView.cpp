#include "ImageAttachmentsView.h"

#include <QDebug>
#include <QWheelEvent>

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
