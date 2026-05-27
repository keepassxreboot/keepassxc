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

#include "ImageAttachmentsWidget.h"

#include "ui_ImageAttachmentsWidget.h"

#include <array>
#include <cmath>
#include <utility>

#include <QDebug>
#include <QEvent>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QPixmap>
#include <QSizeF>
#include <QWheelEvent>

namespace
{
    // Predefined zoom levels must be in ascending order
    constexpr std::array ZoomList = {0.25, 0.5, 0.75, 1.0, 2.0};
    constexpr double WheelZoomStep = 1.1;

    QString formatZoomText(double zoomFactor)
    {
        return QString("%1%").arg(QString::number(zoomFactor * 100, 'f', 0));
    }

    double parseZoomText(const QString& zoomText)
    {
        auto zoomTextTrimmed = zoomText.trimmed();

        if (auto percentIndex = zoomTextTrimmed.indexOf('%'); percentIndex != -1) {
            // Remove the '%' character and parse the number
            zoomTextTrimmed = zoomTextTrimmed.left(percentIndex).trimmed();
        }

        bool ok;
        double zoomFactor = zoomTextTrimmed.toDouble(&ok);
        if (!ok) {
            qWarning() << "Failed to parse zoom text:" << zoomText;
            return std::numeric_limits<double>::quiet_NaN();
        }
        return zoomFactor / 100.0;
    }

} // namespace

ImageAttachmentsWidget::ImageAttachmentsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::ImageAttachmentsWidget)
{
    m_ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    m_ui->imagesView->setScene(m_scene);
    m_ui->imagesView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_ui->imagesView->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    m_ui->imagesView->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    connect(m_ui->imagesView, &ImageAttachmentsView::ctrlWheelEvent, this, &ImageAttachmentsWidget::onWheelZoomEvent);

    static_assert(ZoomList.size() > 0, "ZoomList must not be empty");
    static_assert(ZoomList.front() < ZoomList.back(), "ZoomList must be in ascending order");
    m_zoomHelper = new ZoomHelper(1.0, WheelZoomStep, ZoomList.front(), ZoomList.back(), this);
    connect(m_zoomHelper, &ZoomHelper::zoomChanged, this, &ImageAttachmentsWidget::onZoomFactorChanged);

    initZoomComboBox();
}

ImageAttachmentsWidget::~ImageAttachmentsWidget() = default;

void ImageAttachmentsWidget::initZoomComboBox()
{
    m_ui->zoomComboBox->clear();

    auto fitText = tr("Fit");
    auto textWidth = m_ui->zoomComboBox->fontMetrics().horizontalAdvance(fitText);

    m_ui->zoomComboBox->addItem(fitText, 0.0);

    for (const auto& zoom : ZoomList) {
        auto zoomText = formatZoomText(zoom);
        textWidth = std::max(textWidth, m_ui->zoomComboBox->fontMetrics().horizontalAdvance(zoomText));

        m_ui->zoomComboBox->addItem(zoomText, zoom);
    }

    constexpr int minWidth = 50;
    m_ui->zoomComboBox->setMinimumWidth(textWidth + minWidth);

    connect(m_ui->zoomComboBox, &QComboBox::currentTextChanged, this, &ImageAttachmentsWidget::onZoomChanged);

    connect(m_ui->zoomComboBox->lineEdit(), &QLineEdit::editingFinished, [this]() {
        onZoomChanged(m_ui->zoomComboBox->lineEdit()->text());
    });

    // Fit by default
    m_ui->zoomComboBox->setCurrentIndex(m_ui->zoomComboBox->findData(0.0));
    onZoomChanged(m_ui->zoomComboBox->currentText());
}

void ImageAttachmentsWidget::onWheelZoomEvent(QWheelEvent* event)
{
    m_ui->imagesView->disableAutoFitInView();

    auto finInViewFactor = m_ui->imagesView->calculateFitInViewFactor();
    // Limit the fit-in-view factor to a maximum of 100%
    m_zoomHelper->setMinZoomOutFactor(std::isnan(finInViewFactor) ? 1.0 : std::min(finInViewFactor, 1.0));

    event->angleDelta().y() > 0 ? m_zoomHelper->zoomIn() : m_zoomHelper->zoomOut();
}

void ImageAttachmentsWidget::onZoomFactorChanged(double zoomFactor)
{
    if (m_ui->imagesView->isAutoFitInViewActivated()) {
        return;
    }

    m_ui->imagesView->setTransform(QTransform::fromScale(zoomFactor, zoomFactor));

    // Update the zoom combo box to reflect the current zoom factor
    if (!m_ui->zoomComboBox->lineEdit()->hasFocus()) {
        m_ui->zoomComboBox->setCurrentText(formatZoomText(zoomFactor));
    }
}

void ImageAttachmentsWidget::onZoomChanged(const QString& zoomText)
{
    auto zoomFactor = 1.0;

    if (zoomText == tr("Fit")) {
        m_ui->imagesView->enableAutoFitInView();

        zoomFactor = std::min(m_ui->imagesView->calculateFitInViewFactor(), zoomFactor);
    } else {
        zoomFactor = parseZoomText(zoomText);
        if (!std::isnan(zoomFactor)) {
            m_ui->imagesView->disableAutoFitInView();
        }
    }

    if (std::isnan(zoomFactor)) {
        return;
    }

    m_zoomHelper->setZoomFactor(zoomFactor);
}

void ImageAttachmentsWidget::openAttachment(attachments::Attachment attachment, attachments::OpenMode mode)
{
    m_attachment = std::move(attachment);

    if (mode == attachments::OpenMode::ReadWrite) {
        qWarning() << "Read-write mode is not supported for image attachments";
    }

    loadImage();
}

void ImageAttachmentsWidget::loadImage()
{
    QPixmap pixmap{};
    pixmap.loadFromData(m_attachment.data);
    if (pixmap.isNull()) {
        qWarning() << "Failed to load image from data";
        return;
    }

    m_scene->clear();
    m_scene->addPixmap(std::move(pixmap));
}

attachments::Attachment ImageAttachmentsWidget::getAttachment() const
{
    return m_attachment;
}

// Zoom helper
ZoomHelper::ZoomHelper(double zoomFactor, double step, double min, double max, QObject* parent)
    : QObject(parent)
    , m_step(step)
    , m_minZoomOut(min)
    , m_maxZoomIn(max)
{
    Q_ASSERT(!std::isnan(step) && step > 0);
    Q_ASSERT(!std::isnan(zoomFactor));
    Q_ASSERT(!std::isnan(min));
    Q_ASSERT(!std::isnan(max));
    Q_ASSERT(min < max);

    setZoomFactor(zoomFactor);
}

void ZoomHelper::zoomIn()
{
    const auto newZoomFactor = m_zoomFactor * m_step;
    setZoomFactor(std::isgreater(newZoomFactor, m_maxZoomIn) ? m_zoomFactor : newZoomFactor);
}

void ZoomHelper::zoomOut()
{
    const auto newZoomFactor = m_zoomFactor / m_step;
    setZoomFactor(std::isless(newZoomFactor, m_minZoomOut) ? m_zoomFactor : newZoomFactor);
}

void ZoomHelper::setZoomFactor(double zoomFactor)
{
    if (std::isnan(zoomFactor)) {
        qWarning() << "Failed to set NaN zoom factor";
        return;
    }

    auto oldValue = std::exchange(m_zoomFactor, zoomFactor);
    if (std::isless(oldValue, m_zoomFactor) || std::isgreater(oldValue, m_zoomFactor)) {
        Q_EMIT zoomChanged(m_zoomFactor);
    }
}

double ZoomHelper::getZoomFactor() const
{
    return m_zoomFactor;
}

void ZoomHelper::setMinZoomOutFactor(double zoomFactor)
{
    if (std::isgreater(zoomFactor, m_maxZoomIn)) {
        std::swap(m_maxZoomIn, zoomFactor);
    }

    m_minZoomOut = zoomFactor;
}

void ZoomHelper::setMaxZoomInFactor(double zoomFactor)
{
    if (std::isless(zoomFactor, m_minZoomOut)) {
        std::swap(m_minZoomOut, zoomFactor);
    }

    m_maxZoomIn = zoomFactor;
}
