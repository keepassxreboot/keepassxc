#include "TestWidgetFactory.h"

#include <attachments/AttachmentWidgetFactory.h>
#include <attachments/ImageAttachmentsWidget.h>
#include <attachments/TextAttachmentsWidget.h>
#include <attachments/UnknownAttachmentTypeWidget.h>

#include <core/Tools.h>

#include <memory>

#include <QScopedPointer>
#include <QTest>

void TestAttachmentWidgetFactory::initTestCase()
{
    m_factory = std::make_unique<attachments::AttachmentsWidgetFactory>();
    QVERIFY(m_factory);
}

void TestAttachmentWidgetFactory::testCreateImageWidget()
{
    auto imageWidget = QScopedPointer<attachments::AbstractAttachmentWidget>{
        m_factory->createAttachmentWidget(Tools::MimeType::Image, nullptr)};

    QVERIFY(qobject_cast<ImageAttachmentsWidget*>(imageWidget.data()));
}

void TestAttachmentWidgetFactory::testCreateTextWidget()
{
    for (const auto textType : {Tools::MimeType::Html, Tools::MimeType::Markdown, Tools::MimeType::PlainText}) {
        auto textWidget =
            QScopedPointer<attachments::AbstractAttachmentWidget>{m_factory->createAttachmentWidget(textType, nullptr)};

        QVERIFY(qobject_cast<TextAttachmentsWidget*>(textWidget.data()));
    }
}

void TestAttachmentWidgetFactory::testCreateUnknownWidget()
{
    auto unknownWidget = QScopedPointer<attachments::AbstractAttachmentWidget>{
        m_factory->createAttachmentWidget(Tools::MimeType::Unknown, nullptr)};

    QVERIFY(qobject_cast<UnknownAttachmentTypeWidget*>(unknownWidget.data()));
}
