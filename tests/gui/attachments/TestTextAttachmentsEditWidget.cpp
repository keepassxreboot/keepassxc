#include "TestTextAttachmentsEditWidget.h"

#include <attachments/TextAttachmentsEditWidget.h>

#include <QComboBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTestMouseEvent>
#include <QTextEdit>

void TestTextAttachmentsEditWidget::initTestCase()
{
    m_widget.reset(new TextAttachmentsEditWidget());
}

void TestTextAttachmentsEditWidget::testEmitTextChanged()
{
    QSignalSpy textChangedSignal(m_widget.data(), &TextAttachmentsEditWidget::textChanged);

    m_widget->openAttachment({.name = "test.txt", .data = {}}, attachments::OpenMode::ReadWrite);

    QCoreApplication::processEvents();

    auto textEdit = m_widget->findChild<QTextEdit*>("attachmentsTextEdit");
    QVERIFY(textEdit);

    const QByteArray NewText = "New test text";
    textEdit->setText(NewText);

    QVERIFY(textChangedSignal.count() > 0);
}

void TestTextAttachmentsEditWidget::testEmitPreviewButtonClicked()
{
    QSignalSpy previwButtonClickedSignal(m_widget.data(), &TextAttachmentsEditWidget::previewButtonClicked);

    m_widget->openAttachment({.name = "test.txt", .data = {}}, attachments::OpenMode::ReadWrite);

    QCoreApplication::processEvents();

    auto previewButton = m_widget->findChild<QPushButton*>("previewPushButton");
    QVERIFY(previewButton);

    QTest::mouseClick(previewButton, Qt::LeftButton);

    QCOMPARE(previwButtonClickedSignal.count(), 1);
}
