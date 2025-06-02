#include "TestAttachmentWidget.h"

#include <attachments/AttachmentWidget.h>

#include <attachments/AttachmentTypes.h>
#include <attachments/ImageAttachmentsWidget.h>
#include <attachments/TextAttachmentsWidget.h>

#include <QLabel>
#include <QTest>
#include <QVBoxLayout>

void TestAttachmentsWidget::initTestCase()
{
    m_attachmentWidget.reset(new AttachmentWidget());

    QVERIFY(m_attachmentWidget);
}

void TestAttachmentsWidget::testTextAttachment()
{
    for (const auto& attachment : {attachments::Attachment{.name = "Test.txt", .data = "Test"},
                                   attachments::Attachment{.name = "Test.html", .data = "<h1> test </h1>"},
                                   attachments::Attachment{.name = "Test.md", .data = "**bold**"}}) {
        for (auto mode : {attachments::OpenMode::ReadWrite, attachments::OpenMode::ReadOnly}) {
            m_attachmentWidget->openAttachment(attachment, mode);

            QCoreApplication::processEvents();

            auto layout = m_attachmentWidget->findChild<QVBoxLayout*>("verticalLayout");
            QVERIFY(layout);

            QCOMPARE(layout->count(), 1);

            auto item = layout->itemAt(0);
            QVERIFY(item);

            QVERIFY(qobject_cast<TextAttachmentsWidget*>(item->widget()));

            auto actualAttachment = m_attachmentWidget->getAttachment();
            QCOMPARE(actualAttachment.name, attachment.name);
            QCOMPARE(actualAttachment.data, attachment.data);
        }
    }
}

void TestAttachmentsWidget::testImageAttachment()
{
    const auto Attachment = attachments::Attachment{.name = "Test.jpg", .data = QByteArray::fromHex("FFD8FF")};

    m_attachmentWidget->openAttachment(Attachment, attachments::OpenMode::ReadWrite);

    QCoreApplication::processEvents();

    auto layout = m_attachmentWidget->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY(layout);

    QCOMPARE(layout->count(), 1);

    auto item = layout->itemAt(0);
    QVERIFY(item);

    QVERIFY(qobject_cast<ImageAttachmentsWidget*>(item->widget()));

    auto actualAttachment = m_attachmentWidget->getAttachment();
    QCOMPARE(actualAttachment.name, Attachment.name);
    QCOMPARE(actualAttachment.data, Attachment.data);
}

void TestAttachmentsWidget::testUnknownAttachment()
{
    const auto Attachment = attachments::Attachment{.name = "Test", .data = QByteArray{"ID3"}};

    m_attachmentWidget->openAttachment(Attachment, attachments::OpenMode::ReadWrite);

    QCoreApplication::processEvents();

    auto layout = m_attachmentWidget->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY(layout);

    QCOMPARE(layout->count(), 1);

    auto item = layout->itemAt(0);
    QVERIFY(item);

    auto label = qobject_cast<QLabel*>(item->widget());
    QVERIFY(label);

    QVERIFY(!label->text().isEmpty());

    auto actualAttachment = m_attachmentWidget->getAttachment();
    QCOMPARE(actualAttachment.name, Attachment.name);
    QCOMPARE(actualAttachment.data, Attachment.data);
}
