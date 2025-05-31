#include "TestTextAttachmentsWidget.h"

#include <attachments/TextAttachmentsEditWidget.h>
#include <attachments/TextAttachmentsPreviewWidget.h>
#include <attachments/TextAttachmentsWidget.h>

#include <QPushButton>
#include <QSplitter>
#include <QTest>
#include <QTestMouseEvent>
#include <QTextEdit>
#include <qtestcase.h>
#include <qwidget.h>

void TestTextAttachmentsWidget::initTestCase()
{
    m_textWidget.reset(new TextAttachmentsWidget());
}

void TestTextAttachmentsWidget::testInitTextWidget()
{
    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");

    QCOMPARE(splitter->count(), 2);
    QVERIFY2(qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0)), "EditTextWidget not found");
    QVERIFY2(qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1)), "PreviewTextWidget not found");
}

void TestTextAttachmentsWidget::testTextReadWriteWidget()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadWrite);
    m_textWidget->show();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    auto sizes = splitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY2(sizes[0] > 0, "EditTextWidget width must be greater than zero");

    QCOMPARE(sizes[1], 0);

    auto widget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY(widget);
    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);

    auto previewWidget = qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1));
    QVERIFY(previewWidget);
    attachments = previewWidget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);
}

void TestTextAttachmentsWidget::testTextReadWidget()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadOnly);
    m_textWidget->show();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    auto sizes = splitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY2(sizes[1] > 0, "PreviewTextWidget width must be greater then zero");

    QVERIFY(splitter->widget(0)->isHidden());

    auto widget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY(widget);
    auto attachments = widget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);

    auto previewWidget = qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1));
    QVERIFY(previewWidget);
    attachments = previewWidget->getAttachment();

    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);
}

void TestTextAttachmentsWidget::testTextChanged()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadWrite);

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");

    auto textEdit = editWidget->findChild<QTextEdit*>();
    QVERIFY(textEdit);

    const QByteArray NewText = "New test text";
    textEdit->setText(NewText);

    auto previewWidget = qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1));
    auto attachments = previewWidget->getAttachment();

    QCOMPARE(attachments.data, NewText);
}

void TestTextAttachmentsWidget::testOpenPreviewButton()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadWrite);
    m_textWidget->show();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");

    auto previewButton = editWidget->findChild<QPushButton*>("previewPushButton");

    auto sizes = splitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes[0] > 0);
    QCOMPARE(sizes[1], 0);

    QTest::mouseClick(previewButton, Qt::LeftButton);
    sizes = splitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes[0] > 0);
    QVERIFY(sizes[1] > 0);

    QTest::mouseClick(previewButton, Qt::LeftButton);
    sizes = splitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes[0] > 0);
    QCOMPARE(sizes[1], 0);
}
