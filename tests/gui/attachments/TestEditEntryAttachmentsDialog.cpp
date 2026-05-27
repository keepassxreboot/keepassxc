#include "TestEditEntryAttachmentsDialog.h"

#include <attachments/AttachmentWidget.h>

#include <PreviewEntryAttachmentsDialog.h>

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QSignalSpy>
#include <QSizePolicy>
#include <QTest>
#include <QTestMouseEvent>
#include <QVBoxLayout>

void TestEditEntryAttachmentsDialog::initTestCase()
{
    m_editDialog.reset(new EditEntryAttachmentsDialog());

    QVERIFY(m_editDialog);
}

void TestEditEntryAttachmentsDialog::testSetAttachment()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_editDialog->setAttachment(Test);

    QCoreApplication::processEvents();

    QVERIFY2(m_editDialog->windowTitle().contains(Test.name), "Expected file name in the title");

    auto layout = m_editDialog->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY2(layout, "QVBoxLayout not found");
    QCOMPARE(layout->count(), 2);

    auto widget = qobject_cast<AttachmentWidget*>(layout->itemAt(0)->widget());
    QVERIFY2(widget, "Expected AttachmentWidget");

    auto sizePolicy = widget->sizePolicy();
    QCOMPARE(sizePolicy.horizontalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(sizePolicy.verticalPolicy(), QSizePolicy::Expanding);

    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);
}

void TestEditEntryAttachmentsDialog::testSetAttachmentTwice()
{
    const attachments::Attachment TestText{.name = "text.txt", .data = "Test"};
    m_editDialog->setAttachment(TestText);

    QCoreApplication::processEvents();

    const attachments::Attachment TestImage{
        .name = "test.jpg", .data = QByteArray::fromHex("FFD8FFE000104A46494600010101006000600000FFD9")};
    m_editDialog->setAttachment(TestImage);

    QCoreApplication::processEvents();

    QVERIFY2(m_editDialog->windowTitle().contains(TestImage.name), "Expected file name in the title");

    auto layout = m_editDialog->findChild<QVBoxLayout*>("verticalLayout");
    QVERIFY2(layout, "QVBoxLayout not found");
    QCOMPARE(layout->count(), 2);

    auto widget = qobject_cast<AttachmentWidget*>(layout->itemAt(0)->widget());
    QVERIFY2(widget, "Expected AttachmentWidget");

    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, TestImage.name);
    QCOMPARE(attachments.data, TestImage.data);
}

void TestEditEntryAttachmentsDialog::testBottonsBox()
{
    const attachments::Attachment TestText{.name = "text.txt", .data = "Test"};
    m_editDialog->setAttachment(TestText);

    QCoreApplication::processEvents();

    QSignalSpy acceptButton(m_editDialog.data(), &PreviewEntryAttachmentsDialog::accepted);
    QSignalSpy closeButton(m_editDialog.data(), &PreviewEntryAttachmentsDialog::rejected);

    auto buttonsBox = m_editDialog->findChild<QDialogButtonBox*>();
    QVERIFY2(buttonsBox, "ButtonsBox not found");

    for (auto button : buttonsBox->buttons()) {
        QTest::mouseClick(button, Qt::LeftButton);
    }

    QCOMPARE(acceptButton.count(), 1);
    QCOMPARE(closeButton.count(), 1);
}
