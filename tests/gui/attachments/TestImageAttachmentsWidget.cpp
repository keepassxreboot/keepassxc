#include "TestImageAttachmentsWidget.h"

#include <attachments/ImageAttachmentsView.h>
#include <attachments/ImageAttachmentsWidget.h>

#include <QBuffer>
#include <QComboBox>
#include <QCoreApplication>
#include <QTest>
#include <QTransform>

void TestImageAttachmentsWidget::initTestCase()
{
    m_widget.reset(new ImageAttachmentsWidget());
    m_zoomCombobox = m_widget->findChild<QComboBox*>("zoomComboBox");
    QVERIFY(m_zoomCombobox);

    m_imageAttachmentsView = m_widget->findChild<ImageAttachmentsView*>("imagesView");
    QVERIFY(m_imageAttachmentsView);

    // Generate the black rectange.
    QImage image(1000, 1000, QImage::Format_RGB32);
    image.fill(Qt::black);

    QByteArray imageBytes{};
    QBuffer buffer(&imageBytes);
    buffer.open(QIODevice::WriteOnly);

    image.save(&buffer, "PNG");

    m_widget->openAttachment({.name = "black.png", .data = std::move(imageBytes)}, attachments::OpenMode::ReadOnly);

    m_widget->show();

    QCoreApplication::processEvents();
}

void TestImageAttachmentsWidget::testFitInView()
{
    QCOMPARE(m_zoomCombobox->currentText(), tr("Fit"));
    QVERIFY(m_imageAttachmentsView->isAutoFitInViewActivated());

    auto zoomFactor = m_imageAttachmentsView->transform();

    m_widget->setMinimumSize(m_widget->size() + QSize{100, 100});

    QCoreApplication::processEvents();

    QVERIFY(zoomFactor != m_imageAttachmentsView->transform());
}

void TestImageAttachmentsWidget::testZoomCombobox()
{
    for (const auto zoom : {0.25, 0.5, 1.0, 2.0}) {
        auto index = m_zoomCombobox->findData(zoom);
        QVERIFY(index != -1);

        m_zoomCombobox->setCurrentIndex(index);

        QCoreApplication::processEvents();

        QCOMPARE(m_imageAttachmentsView->transform(), QTransform::fromScale(zoom, zoom));
    }
}

void TestImageAttachmentsWidget::testEditZoomCombobox()
{
    for (double i = 0.25; i < 5; i += 0.25) {
        m_zoomCombobox->setCurrentText(QString::number(i * 100));

        QCoreApplication::processEvents();

        QCOMPARE(m_imageAttachmentsView->transform(), QTransform::fromScale(i, i));
    }
}

void TestImageAttachmentsWidget::testEditWithPercentZoomCombobox()
{
    // Example 100 %
    for (double i = 0.25; i < 5; i += 0.25) {
        m_zoomCombobox->setCurrentText(QString("%1 %").arg(i * 100));

        QCoreApplication::processEvents();

        QCOMPARE(m_imageAttachmentsView->transform(), QTransform::fromScale(i, i));
    }

    // Example 100%
    for (double i = 0.25; i < 5; i += 0.25) {
        m_zoomCombobox->setCurrentText(QString("%1%").arg(i * 100));

        QCoreApplication::processEvents();

        QCOMPARE(m_imageAttachmentsView->transform(), QTransform::fromScale(i, i));
    }
}

void TestImageAttachmentsWidget::testInvalidValueZoomCombobox()
{
    auto index = m_zoomCombobox->findData(1.0);
    QVERIFY(index != -1);

    m_zoomCombobox->setCurrentIndex(index);

    QCoreApplication::processEvents();

    const QTransform expectedTransform = m_imageAttachmentsView->transform();

    for (const auto& invalidValue : {"Help", "3,4", "", ".", "% 100"}) {
        m_zoomCombobox->setCurrentText(invalidValue);

        QCoreApplication::processEvents();

        QCOMPARE(m_imageAttachmentsView->transform(), expectedTransform);
    }
}

void TestImageAttachmentsWidget::testZoomInByMouse()
{
    QPoint center = m_imageAttachmentsView->rect().center();

    // Set zoom: 100%
    auto index = m_zoomCombobox->findData(1.0);
    QVERIFY(index != -1);
    m_zoomCombobox->setCurrentIndex(index);

    m_imageAttachmentsView->setFocus();

    QCoreApplication::processEvents();

    const auto transform = m_imageAttachmentsView->transform();

    QWheelEvent event(center, // local pos
                      m_imageAttachmentsView->mapToGlobal(center), // global pos
                      QPoint(0, 0),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::ScrollBegin,
                      false);

    QCoreApplication::sendEvent(m_imageAttachmentsView->viewport(), &event);

    QCoreApplication::processEvents();

    QTransform t = m_imageAttachmentsView->transform();
    QVERIFY(t.m11() > transform.m11());
    QVERIFY(t.m22() > transform.m22());
}

void TestImageAttachmentsWidget::testZoomOutByMouse()
{
    QPoint center = m_imageAttachmentsView->rect().center();

    // Set zoom: 100%
    auto index = m_zoomCombobox->findData(1.0);
    QVERIFY(index != -1);
    m_zoomCombobox->setCurrentIndex(index);

    m_imageAttachmentsView->setFocus();

    QCoreApplication::processEvents();

    const auto transform = m_imageAttachmentsView->transform();

    QWheelEvent event(center, // local pos
                      center, // global pos
                      QPoint(0, 0),
                      QPoint(0, -120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::ScrollBegin,
                      true);

    QCoreApplication::sendEvent(m_imageAttachmentsView->viewport(), &event);

    QCoreApplication::processEvents();

    QTransform t = m_imageAttachmentsView->transform();
    QVERIFY(t.m11() < transform.m11());
    QVERIFY(t.m22() < transform.m22());
}

void TestImageAttachmentsWidget::testZoomLowerBound()
{
    m_widget->setMinimumSize(100, 100);

    QCoreApplication::processEvents();

    auto minFactor = m_imageAttachmentsView->calculateFitInViewFactor();

    // Set size less then minFactor
    m_zoomCombobox->setCurrentText(QString::number((minFactor * 100.0) / 2));

    QCoreApplication::processEvents();

    const auto expectTransform = m_imageAttachmentsView->transform();

    QPoint center = m_imageAttachmentsView->rect().center();

    QWheelEvent event(center, // local pos
                      center, // global pos
                      QPoint(0, 0),
                      QPoint(0, -120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::ScrollBegin,
                      true);

    QCoreApplication::sendEvent(m_imageAttachmentsView->viewport(), &event);

    QCoreApplication::processEvents();

    QCOMPARE(m_imageAttachmentsView->transform(), expectTransform);
}

void TestImageAttachmentsWidget::testZoomUpperBound()
{
    m_widget->setMinimumSize(100, 100);

    // Set size less then minFactor
    m_zoomCombobox->setCurrentText(QString::number(500));

    QCoreApplication::processEvents();

    const auto expectTransform = m_imageAttachmentsView->transform();

    QPoint center = m_imageAttachmentsView->rect().center();

    QWheelEvent event(center, // local pos
                      center, // global pos
                      QPoint(0, 0),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::ScrollBegin,
                      true);

    QCoreApplication::sendEvent(m_imageAttachmentsView->viewport(), &event);

    QCoreApplication::processEvents();

    QCOMPARE(m_imageAttachmentsView->transform(), expectTransform);
}
