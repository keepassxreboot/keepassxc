#include "TestImageAttachmentsView.h"

#include <attachments/ImageAttachmentsView.h>

#include <QBuffer>
#include <QPixmap>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

void TestImageAttachmentsView::initTestCase()
{
    m_view.reset(new ImageAttachmentsView());

    // Generate the black rectange.
    QImage image(1000, 1000, QImage::Format_RGB32);
    image.fill(Qt::black);

    auto scene = new QGraphicsScene();
    scene->addPixmap(QPixmap::fromImage(image));

    m_view->setScene(scene);
    m_view->show();

    QCoreApplication::processEvents();
}

void TestImageAttachmentsView::testEmitWheelEvent()
{
    QSignalSpy ctrlWheelEvent{m_view.data(), &ImageAttachmentsView::ctrlWheelEvent};

    QPoint center = m_view->rect().center();

    m_view->setFocus();

    QWheelEvent event(center, // local pos
                      m_view->mapToGlobal(center), // global pos
                      QPoint(0, 0),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::ScrollBegin,
                      false);

    QCoreApplication::sendEvent(m_view->viewport(), &event);

    QCOMPARE(ctrlWheelEvent.count(), 1);
}

void TestImageAttachmentsView::testEnableFit()
{
    m_view->enableAutoFitInView();
    QVERIFY(m_view->isAutoFitInViewActivated());

    const auto oldTransform = m_view->transform();

    m_view->resize(m_view->size() + QSize(100, 100));

    QCoreApplication::processEvents();

    QVERIFY(m_view->transform() != oldTransform);
}

void TestImageAttachmentsView::testDisableFit()
{
    m_view->disableAutoFitInView();
    QVERIFY(!m_view->isAutoFitInViewActivated());

    const auto expectedTransform = m_view->transform();

    m_view->resize(m_view->size() + QSize(100, 100));

    QCoreApplication::processEvents();

    QCOMPARE(m_view->transform(), expectedTransform);
}
