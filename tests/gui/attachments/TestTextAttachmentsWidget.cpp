#include "TestTextAttachmentsWidget.h"

#include <attachments/TextAttachmentsEditWidget.h>
#include <attachments/TextAttachmentsPreviewWidget.h>
#include <attachments/TextAttachmentsWidget.h>

#include <QPushButton>
#include <QSignalSpy>
#include <QSplitter>
#include <QTest>
#include <QTestMouseEvent>
#include <QTextEdit>
#include <QTimer>

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

    QCoreApplication::processEvents();

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

    // Find preview widget and verify that it is in collapsed state
    auto previewWidget = qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1));
    QVERIFY(previewWidget);
    QCOMPARE(previewWidget->width(), 0);

    // Show the preview widget
    QTest::mouseClick(m_textWidget->findChild<QPushButton*>("previewPushButton"), Qt::LeftButton);
    QCoreApplication::processEvents();
    QVERIFY(previewWidget->width() > 0);

    // Verify attachment data has been loaded in the preview
    attachments = previewWidget->getAttachment();
    QCOMPARE(attachments.name, Test.name);
    QCOMPARE(attachments.data, Test.data);
}

void TestTextAttachmentsWidget::testTextReadWidget()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadOnly);
    m_textWidget->show();

    QCoreApplication::processEvents();

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

    QCoreApplication::processEvents();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");

    auto textEdit = editWidget->findChild<QTextEdit*>();
    QVERIFY(textEdit);

    const QByteArray NewText = "New test text";
    textEdit->setText(NewText);

    QCoreApplication::processEvents();

    auto attachments = m_textWidget->getAttachment();

    QCOMPARE(attachments.data, NewText);
}

void TestTextAttachmentsWidget::testTextChangedInReadOnlyMode()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadOnly);

    QCoreApplication::processEvents();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");

    auto textEdit = editWidget->findChild<QTextEdit*>();
    QVERIFY(textEdit);

    const QByteArray NewText = "New test text";
    textEdit->setText(NewText);

    QCoreApplication::processEvents();

    auto attachments = m_textWidget->getAttachment();

    QCOMPARE(attachments.data, Test.data);
}

void TestTextAttachmentsWidget::testPreviewTextChanged()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};

    auto previewTimer = m_textWidget->findChild<QTimer*>();
    QVERIFY2(previewTimer, "PreviewTimer not found!");

    QSignalSpy timeout(previewTimer, &QTimer::timeout);

    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadWrite);

    // Waiting for the first timeout
    while (timeout.count() < 1) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    splitter->setSizes({1, 1});

    QCoreApplication::processEvents();

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");

    auto textEdit = editWidget->findChild<QTextEdit*>();
    QVERIFY(textEdit);

    const QByteArray NewText = "New test text";
    textEdit->setText(NewText);

    // Waiting for the second timeout
    while (timeout.count() < 2) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    auto previewWidget = qobject_cast<TextAttachmentsPreviewWidget*>(splitter->widget(1));
    auto attachments = previewWidget->getAttachment();

    QCOMPARE(attachments.data, NewText);
}

void TestTextAttachmentsWidget::testOpenPreviewButton()
{
    const attachments::Attachment Test{.name = "text.txt", .data = "Test"};
    m_textWidget->openAttachment(Test, attachments::OpenMode::ReadWrite);
    m_textWidget->show();

    QCoreApplication::processEvents();

    auto splitter = m_textWidget->findChild<QSplitter*>();
    QVERIFY2(splitter, "Splitter not found");
    QCOMPARE(splitter->sizes().size(), 2);

    auto editWidget = qobject_cast<TextAttachmentsEditWidget*>(splitter->widget(0));
    QVERIFY2(editWidget, "Edit widget not found");
    QVERIFY(editWidget->isVisible());

    auto previewButton = editWidget->findChild<QPushButton*>("previewPushButton");

    auto sizes = splitter->sizes();
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
