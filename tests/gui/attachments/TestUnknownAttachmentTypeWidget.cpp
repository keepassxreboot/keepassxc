#include "TestUnknownAttachmentTypeWidget.h"

#include <QLabel>
#include <QTest>

void TestUnknownAttachmentTypeWidget::initTestCase()
{
    m_unknownWidget.reset(new UnknownAttachmentTypeWidget());
}

void TestUnknownAttachmentTypeWidget::testUserMessage()
{
    auto messageLabel = m_unknownWidget->findChild<QLabel*>("MessageLabel");
    QVERIFY2(messageLabel, "MessageLabel not found");

    QCOMPARE(messageLabel->text(), tr("Unknown attachment type"));
}
