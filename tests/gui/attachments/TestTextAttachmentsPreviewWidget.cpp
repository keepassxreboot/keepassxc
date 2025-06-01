#include "TestTextAttachmentsPreviewWidget.h"

#include <attachments/TextAttachmentsPreviewWidget.h>

#include <QComboBox>
#include <QTest>

void TestTextAttachmentsPreviewWidget::initTestCase()
{
    m_widget.reset(new TextAttachmentsPreviewWidget());
}

void TestTextAttachmentsPreviewWidget::testDetectMimeByFile()
{
    const auto combobox = m_widget->findChild<QComboBox*>("typeComboBox");
    QVERIFY(combobox);

    const attachments::Attachment Text{.name = "test.txt", .data = {}};
    m_widget->openAttachment(Text, attachments::OpenMode::ReadOnly);

    QCoreApplication::processEvents();

    QCOMPARE(combobox->currentData().toInt(), TextAttachmentsPreviewWidget::PlainText);

    const attachments::Attachment Html{.name = "test.html", .data = {}};
    m_widget->openAttachment(Html, attachments::OpenMode::ReadOnly);

    QCoreApplication::processEvents();

    QCOMPARE(combobox->currentData().toInt(), TextAttachmentsPreviewWidget::Html);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    const attachments::Attachment Markdown{.name = "test.md", .data = {}};
    m_widget->openAttachment(Markdown, attachments::OpenMode::ReadOnly);

    QCoreApplication::processEvents();

    QCOMPARE(combobox->currentData().toInt(), TextAttachmentsPreviewWidget::Markdown);
#endif
}
