#include "TestPreviewEntryAttachmentsDialog.h"

#include <attachments/AttachmentWidget.h>

#include <PreviewEntryAttachmentsDialog.h>

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QSignalSpy>
#include <QSizePolicy>
#include <QTest>
#include <QTestMouseEvent>
#include <QVBoxLayout>

void TestPreviewEntryAttachmentsDialog::initTestCase()
{
    m_previewDialog.reset(new PreviewEntryAttachmentsDialog());

    QVERIFY(m_previewDialog);
}

void TestPreviewEntryAttachmentsDialog::testSetAttachment()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_previewDialog->setAttachment(Test);

    QCoreApplication::processEvents();

    QVERIFY2(m_previewDialog->windowTitle().contains(Test.name), "Expected file name in the title");

    auto layout = m_previewDialog->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY2(layout, "QVBoxLayout not found");
    QCOMPARE(layout->count(), 2);

    auto widget = qobject_cast<AttachmentWidget*>(layout->itemAt(0)->widget());
    QVERIFY2(widget, "Expected AbstractAttachmentWidget");

    auto sizePolicy = widget->sizePolicy();
    QCOMPARE(sizePolicy.horizontalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(sizePolicy.verticalPolicy(), QSizePolicy::Expanding);

    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);
}

void TestPreviewEntryAttachmentsDialog::testSetAttachmentTwice()
{
    const attachments::Attachment TestText{.name = "text.txt", .data = "Test"};
    m_previewDialog->setAttachment(TestText);

    QCoreApplication::processEvents();

    const attachments::Attachment TestImage{
        .name = "test.jpg", .data = QByteArray::fromHex("FFD8FFE000104A46494600010101006000600000FFD9")};
    m_previewDialog->setAttachment(TestImage);

    QCoreApplication::processEvents();

    QVERIFY2(m_previewDialog->windowTitle().contains(TestImage.name), "Expected file name in the title");

    auto layout = m_previewDialog->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY2(layout, "QVBoxLayout not found");
    QCOMPARE(layout->count(), 2);

    auto widget = qobject_cast<AttachmentWidget*>(layout->itemAt(0)->widget());
    QVERIFY2(widget, "Expected AbstractAttachmentWidget");

    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, TestImage.name);
    QCOMPARE(attachments.data, TestImage.data);
}

void TestPreviewEntryAttachmentsDialog::testBottonsBox()
{
    const attachments::Attachment TestText{.name = "text.txt", .data = "Test"};
    m_previewDialog->setAttachment(TestText);

    QCoreApplication::processEvents();

    QSignalSpy saveButton(m_previewDialog.data(), &PreviewEntryAttachmentsDialog::saveAttachment);
    QSignalSpy openButton(m_previewDialog.data(), &PreviewEntryAttachmentsDialog::openAttachment);
    QSignalSpy closeButton(m_previewDialog.data(), &PreviewEntryAttachmentsDialog::rejected);

    auto buttonsBox = m_previewDialog->findChild<QDialogButtonBox*>("dialogButtons");
    QVERIFY2(buttonsBox, "ButtonsBox not found");

    for (auto button : buttonsBox->buttons()) {
        QTest::mouseClick(button, Qt::LeftButton);
    }

    QCOMPARE(saveButton.count(), 1);
    QCOMPARE(openButton.count(), 1);
    QCOMPARE(closeButton.count(), 1);
}
